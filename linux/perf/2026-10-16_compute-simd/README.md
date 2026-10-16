# 2026-10-16 compute-simd: `-O3 -ffast-math`로 나눗셈을 SIMD화하면 빨라지는가

## 목적
`compute()`는 스칼라 나눗셈 `vdivsd`의 처리량(4.5사이클/개)에 묶여 있다(기준선, `2026-10-15_demo-baseline`).
누산기 4개로 나눠도 효과가 없었다(`2026-10-16_compute-4acc`).
이번에는 **나눗셈 한 번에 double 4개를 처리하는 `vdivpd ymm`** 을 쓰게 해서 나눗셈 횟수 자체를 줄인다.

## 환경과 빌드
- 기준선과 같다: AMD Ryzen 5 7430U (Zen 3, AVX2 있음, **AVX-512 없음**), perf 6.12, gcc 14.3.1, `perf_event_paranoid=2`
- 이 시스템(EL10)의 gcc는 기본이 `-march=x86-64-v3`이라 `-mavx2`를 주지 않아도 AVX2(ymm)를 쓴다 (`gcc -Q --help=target`으로 확인).
- 빌드는 모두 `gcc -O1 -g -fno-omit-frame-pointer <소스> -o <바이너리>`다. 다른 함수(branchy, sum_cols 등)가 바뀌지 않도록 `-O3 -ffast-math`는 **compute에만** 속성으로 걸었다.
  ```c
  __attribute__((optimize("O3","fast-math")))
  double compute(void) { ... }
  ```
- 바이너리(`demo`, `demo_long`, `demo_base`)는 측정 후 지웠다. `perf.data`의 빌드 ID는 `344e9100…`이다.

### 처음 시도: `-O3 -ffast-math`만으로는 SIMD가 안 된다
원본 코드(`long i`)에 플래그만 주면 어셈블리가 **원본과 똑같은 스칼라 코드**로 나온다. gcc에 이유를 물으면:
```
$ gcc -O3 -ffast-math -fopt-info-vec-all -S c.c
c.c:1:8: missed: not vectorized: unsupported data-type double
```
- `long`(64비트 정수)을 double 4개로 한꺼번에 바꾸는 명령 `vcvtqq2pd`는 **AVX-512DQ** 전용이고, Zen 3에는 없다.
- 그래서 카운터를 **`int`** 로 바꿨다. 32비트 정수를 바꾸는 `vcvtdq2pd`는 AVX에 있다. i < 2억이라 int 범위(약 21억) 안이다.
- 기준선 README의 "`-O3 -ffast-math`로 SIMD화하면 vdivpd로 4개씩"은 **이 조건이 빠진** 설명이었다.

## 파일 목록
| 파일 | 내용 | 만든 명령 |
|---|---|---|
| `demo.c` | 측정한 소스: compute에 속성 추가 + 카운터 `int` (SIMD 됨) | - |
| `demo_long.c` | 대조군: compute에 속성만 추가하고 카운터는 `long` (SIMD 안 됨) | - |
| `stat.txt` | demo (SIMD) 통계 3회 평균 | `perf stat -r 3 -e cycles,instructions,cache-misses,cache-references,branches,branch-misses ./demo 2> stat.txt` |
| `stat_long.txt` | demo_long 통계 | 같은 `perf stat`을 `./demo_long`으로 |
| `stat_base.txt` | 원본을 같은 날 다시 측정 | `gcc ... ../../demo.c -o demo_base` 후 같은 `perf stat`을 `./demo_base`로 |
| `perf.data` | demo (SIMD) 샘플링 원본 | `perf record -g -o perf.data ./demo` |
| `report.txt` | 함수별 비율 | `perf report -i perf.data --stdio --no-children` |
| `annotate_branchy.txt`, `annotate_compute.txt`, `annotate_sum_cols.txt` | 상위 핫스팟 함수의 명령어별 샘플 (branchy, sum_cols는 원본과 같은 코드) | `perf annotate -i perf.data <함수> --stdio` |

---

## 결과 1: `perf stat`

| 지표 | 원본 (stat_base) | 플래그만, long (stat_long) | **SIMD, int (stat)** |
|---|---|---|---|
| 실행 시간 | 0.8677초 ±0.12% | 0.8643초 ±0.14% | **0.7044초** ±0.14% |
| cycles | 3.660G | 3.647G | **2.959G** |
| instructions | 3.611G | 3.626G | **2.701G** |
| IPC | 0.99 | 0.99 | 0.91 |
| branches | 902.7M | 904.2M | **727.4M** |
| branch-misses | 84.08M | 84.09M | 84.10M |

### 알 수 있는 것
- **SIMD 버전은 0.163초(-18.8%) 빨라졌다.** 처음으로 compute가 실제로 빨라졌다.
- **플래그만 준 long 버전은 원본과 같다.** 어셈블리가 같으니 당연하다. 플래그를 줬다고 SIMD가 되는 것은 아니다. **annotate나 `-fopt-info-vec`로 실제로 됐는지 확인해야 한다.**
- **instructions -0.91G 검산:** 원본 루프는 원소당 6개라 2억 × 6 = 12억 개다. SIMD 루프는 한 바퀴에 원소 8개를 처리하고 명령어는 12개다(아래 annotate). 2,500만 바퀴 × 12 = 3억 개다. 차이 9억 개는 측정값 9.1억과 맞는다.
- **branches -175M 검산:** 루프 분기가 2억 번에서 2,500만 번이 됐다. 차이 1.75억은 측정값 1.753억과 맞는다.
- branch-misses는 그대로 8,400만 번이다. branchy는 건드리지 않았기 때문이다.
- IPC가 0.91로 여전히 1보다 낮다. 나눗셈기를 기다린다는 사정은 같고, **한 번 기다릴 때 처리하는 양이 4배가 된 것**이다.

---

## 결과 2: `perf report` (report.txt)

```
72.57%  branchy     ← 기준선 58% (자기 시간은 그대로인데 전체가 줄어서 비율이 올라감)
 7.58%  compute     ← 기준선 24.57%
 7.38%  __random
 7.24%  sum_cols
```

- compute 사이클: 기준선 3.67G × 24.57% ≈ **0.90G**에서 이번 2.959G × 7.58% = **0.224G**로 줄었다. **약 4.0배** 빨라졌다.
- 예상 절감량 0.90 − 0.224 = 0.68G 사이클과 전체 cycles 차이 0.70G(3.660 − 2.959)가 거의 같다. 줄어든 시간은 전부 compute에서 나왔다.
- 이제 branchy가 73%다. 다음 개선 대상은 더욱 분명히 branchy다.

---

## 결과 3: `perf annotate` (annotate_compute.txt)

```
vmovdqa      %ymm2,%ymm0              ← i 8개 (int, ymm 한 개에 8개)
addl         $0x1,%eax
vpaddd       %ymm5,%ymm2,%ymm2        ← i += 8 (8개 동시에)
vcvtdq2pd    %xmm0,%ymm1              ← 아래쪽 int 4개 → double 4개
vdivpd       %ymm1,%ymm4,%ymm1        ← 1.0 / (4개)
vextracti128 $0x1,%ymm0,%xmm0  28.57% ← 위쪽 int 4개 꺼내기
vcvtdq2pd    %xmm0,%ymm0        5.71%
vdivpd       %ymm0,%ymm4,%ymm0        ← 1.0 / (4개)
vaddpd       %ymm0,%ymm1,%ymm0 36.67%
vaddpd       %ymm0,%ymm3,%ymm3        ← x(4갈래) += ...
cmpl         $0x17d783f,%eax   29.04% ← 0x17d783f = 24,999,999바퀴
jne
```

### 알 수 있는 것
- 샘플이 **두 `vdivpd` 바로 뒤**에 몰려 있다(skid). 여전히 나눗셈을 기다리는 시간이 대부분이다.
- `-ffast-math`가 있어서 gcc가 덧셈 순서를 바꿀 수 있었다. 그래서 `x`를 ymm 하나에 4갈래로 나눠 더하고(`vaddpd %ymm0,%ymm3,%ymm3`) 끝에서 합친다. **누산기 분리를 컴파일러가 자동으로 한 것**이다. 순서를 바꾸면 결과가 달라질 수 있어서, `-ffast-math` 없이는 이 벡터화가 허용되지 않는다.
- **사이클 검산:** 0.224G 사이클 ÷ `vdivpd` 5,000만 번(2,500만 바퀴 × 2) = **vdivpd 1개당 4.49사이클**이다. 스칼라 `vdivsd`(4.49사이클)와 같다.
  - 즉 Zen 3에서는 **나눗셈 1번의 처리량이 스칼라든 ymm 4개짜리든 같다.** 그래서 4개씩 하면 정확히 4배가 된다.
  - 원소 1개당으로는 4.5사이클에서 **1.12사이클**이 됐다.

---

## 결과의 정확도
`-ffast-math`는 덧셈 순서를 바꾸므로 결과가 미세하게 달라진다. 별도 프로그램에서 `%.17g`로 비교했다(스크래치 확인, 파일로는 남기지 않음).
```
원본 : 19.691043586914379
SIMD : 19.691043586915896   (차이 1.5e-12, 상대오차 약 8e-14)
```
demo의 출력(`%f`, 소수 6자리)은 세 버전 모두 `19.691044`로 같다. 이 용도에서는 문제없는 수준이다.

---

## 기준선과 비교
| | 기준선 | 4누산기 | **SIMD** |
|---|---|---|---|
| 전체 시간 | 0.868초 | 0.867초 | **0.704초** |
| compute 비율 | 24.57% | 24.58% | **7.58%** |
| compute 사이클 | 0.90G | 0.90G | **0.224G** |
| 나눗셈 명령 1개당 사이클 | 4.5 | 4.49 | 4.49 (단, 4개씩) |
| 원소당 사이클 | 4.5 | 4.49 | **1.12** |

## 결론
- **SIMD화하면 compute가 4.0배, 전체가 18.8% 빨라진다.** 나눗셈기 처리량이 병목일 때는 **나눗셈 1번에 더 많은 일을 시키는 것**이 답이다. 사슬을 나누는 누산기 분리로는 안 됐다.
- 다만 `-O3 -ffast-math`만으로는 SIMD가 되지 않았다. Zen 3에는 `long → double` 벡터 변환 명령이 없어서 카운터를 `int`로 바꿔야 했다. **최적화 플래그의 효과는 꼭 어셈블리로 확인한다.**
- `-ffast-math`는 결과를 미세하게 바꾼다(이번에는 상대오차 8e-14). 정확한 재현이 필요한 계산에는 주의한다.

## 다음 할 일
1. **branchy (이제 73%)**: branchless 코드(`s += (v[i] < 128) * v[i]`)나 cmov를 허용해서 측정한다. 가장 큰 효과가 남아 있다.
2. **sum_cols**: 루프 순서를 바꾼다.
3. (선택) `-ffast-math` 없이 같은 효과를 내는 방법(손으로 쓴 intrinsics, 또는 `-fassociative-math`만 켜기)과 비교한다.
