# 2026-10-16 combined: compute-simd + branchless를 합치면 효과가 그대로 더해지는가

## 목적
따로 측정한 두 개선을 한 프로그램에 합친다.
- `2026-10-16_compute-simd`: compute를 SIMD로 (전체 −0.163초)
- `2026-10-16_branchless`: branchy를 branchless로 (전체 −0.424초)

두 함수는 서로 독립이니 **효과가 그대로 더해져야** 한다. 예측은 0.442초 − 0.163초 ≈ **0.28초**다. 맞는지 확인한다.

## 환경과 빌드
- 기준선과 같다: AMD Ryzen 5 7430U, perf 6.12, gcc 14.3.1 (기본 `-march=x86-64-v3`), `perf_event_paranoid=2`
- 빌드: `gcc -O1 -g -fno-omit-frame-pointer demo.c -o demo` (이 폴더에서)
- 소스는 `branchless/demo.c`에 `compute-simd`의 compute 변경을 그대로 넣은 것이다. `compute-simd/demo.c`와 diff를 떠 보면 branchy 부분만 다르다.
- 어셈블리 확인: branchy에 `cmovg`가 있고 `jg`가 없다. compute에 `vdivpd`가 2개 있다. 두 변경 모두 들어갔다.
- 출력은 원본과 같다: `42928701440.000000 42928701440.000000 5327791670 19.691044`
- 바이너리(`demo`, `demo_base`)는 측정 후 지웠다. `perf.data`의 빌드 ID는 `4613a18f…`이다.

## 파일 목록
| 파일 | 내용 | 만든 명령 |
|---|---|---|
| `demo.c` | 측정한 소스 (branchy branchless + compute SIMD) | - |
| `stat.txt` | 합친 버전 통계 3회 평균 | `perf stat -r 3 -e cycles,instructions,cache-misses,cache-references,branches,branch-misses ./demo 2> stat.txt` |
| `stat_base.txt` | 원본을 같은 날 다시 측정 | `gcc -O1 -g -fno-omit-frame-pointer ../../demo.c -o demo_base` 후 같은 `perf stat`을 `./demo_base`로 |
| `perf.data` | 샘플링 원본 | `perf record -g -o perf.data ./demo` |
| `report.txt` | 함수별 비율 | `perf report -i perf.data --stdio --no-children` |
| `annotate_branchy.txt`, `annotate_compute.txt`, `annotate_sum_cols.txt` | 상위 핫스팟 함수 명령어별 샘플 (branchy, compute, sum_cols 순) | `perf annotate -i perf.data <함수> --stdio` |

---

## 결과 1: `perf stat`

| 지표 | 원본 (stat_base) | branchless만 | SIMD만 | **합친 버전 (stat)** |
|---|---|---|---|---|
| 실행 시간 | 0.8681초 | 0.4421초 | 0.7044초 | **0.2806초** ±0.46% |
| cycles | 3.662G | 1.832G | 2.959G | **1.142G** |
| instructions | 3.610G | 3.693G | 2.701G | **2.783G** |
| IPC | 0.99 | 2.02 | 0.91 | **2.44** |
| branches | 902.2M | 648.9M | 727.4M | **472.2M** |
| branch-misses | 84.0M | 1만 | 84.1M | **9천** |

(branchless만, SIMD만 열은 각 폴더의 `stat.txt`에서 가져왔다.)

### 알 수 있는 것
- **예측 0.28초, 측정 0.2806초.** 원본보다 **3.09배** 빠르다.
- **cycles가 더해지는지 검산:** 원본에서 줄어든 cycles는 3.662 − 1.142 = **2.520G**다. 각각의 절감량은 branchless 1.824G(3.656 − 1.832)와 SIMD 0.701G(3.660 − 2.959)이고, 합하면 **2.525G**다. 0.2% 차이로 맞는다.
- **instructions 검산:** branchless는 +89.6M, SIMD는 −910M이라 합하면 −820M이다. 측정 차이는 3.610 − 2.783 = **−827M**으로 맞는다.
- **branches 검산:** branchless −251.7M과 SIMD −175M을 합하면 −426.7M이다. 측정은 **−430M**으로 맞는다.
- 세 숫자가 모두 단순 합과 맞는다. 두 개선이 **서로 간섭하지 않는다**는 뜻이다. 다른 함수, 다른 실행 유닛(정수 분기 쪽과 FP 나눗셈기)을 쓰고 실행 시간도 겹치지 않기 때문이다.
- IPC 2.44: 이제 프로그램 전체에서 CPU가 노는 시간이 많이 줄었다.

---

## 결과 2: `perf report` (report.txt)

| 함수 | 비율 | 사이클 (1.142G × 비율) | 따로 측정했을 때 |
|---|---|---|---|
| branchy | 29.59% | **0.338G** | branchless: 0.337G |
| compute | 19.59% | **0.224G** | compute-simd: 0.224G |
| sum_cols | 18.09% | 0.207G | 기준선: 0.212G (안 바꿈) |
| `__random` + `__random_r` + `rand` | 25.39% | 0.290G | 초기화용 (측정 대상 아님) |
| main | 6.05% | 0.069G | |
| sum_rows | 1.21% | 0.014G | |

### 알 수 있는 것
- **함수별 사이클이 따로 측정한 값과 거의 정확히 같다.** branchy 0.338G 대 0.337G, compute 0.224G 대 0.224G다. 비율(%)은 전체가 줄면서 바뀌었지만 각 함수가 쓴 절대 시간은 그대로다. **비율이 아니라 사이클로 비교해야** 이것이 보인다.
- 바꾸지 않은 sum_cols(0.207G)도 기준선(0.212G)과 같다. 측정이 안정적이라는 확인도 된다.
- **이제 시간을 차지하는 순서는 branchy 0.34G > rand 초기화 0.29G > compute 0.22G > sum_cols 0.21G다.** 고르게 나뉘어서, 하나만 고쳐서 크게 줄일 곳은 없다.
- rand 초기화(1,600만 번 `rand()` 호출)가 2위가 됐다. 측정 대상은 아니지만 실제 프로그램이라면 무시할 수 없는 크기다.

---

## 기준선과 비교
| | 기준선 (2026-10-15) | 합친 버전 |
|---|---|---|
| 전체 시간 | 0.869초 (오늘 재측정 0.868초) | **0.281초 (3.09배)** |
| IPC | 0.98 | **2.44** |
| branch-misses | 8,400만 | **9천** |
| branchy 사이클 | 2.15G | 0.338G (6.4배) |
| compute 사이클 | 0.90G | 0.224G (4.0배) |

## 결론
- **두 개선의 효과는 그대로 더해진다.** 예측 0.28초에 측정 0.2806초이고, cycles, instructions, branches 모두 단순 합과 맞는다.
- 함수별 효과를 따로 측정해 두면 합쳤을 때의 결과를 미리 계산할 수 있다. 단, 이번처럼 **함수들이 서로 자원을 나눠 쓰지 않을 때**만 그렇다. 같은 메모리 대역폭을 나눠 쓰는 경우라면 더해지지 않을 수 있다.
- 기준선의 세 가지 느린 패턴 중 두 개(분기 예측 실패, 나눗셈 처리량)를 해결했다.

## 다음 할 일
1. **sum_cols**: 루프 순서를 행 방향으로 바꾼다(기준선에서 sum_rows보다 15배 느림). 0.207G 사이클 중 대부분을 줄일 수 있을 것으로 예상한다.
2. branchy 원소당 2.0사이클이 메모리 대역폭 때문인지 확인한다(`branchless/README.md` 참고).
3. (선택) rand 초기화를 더 빠른 난수 생성기(예: xorshift)로 바꾸는 것이 얼마나 효과가 있는지 본다.
