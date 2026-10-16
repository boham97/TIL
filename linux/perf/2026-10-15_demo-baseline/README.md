# 2026-10-15 demo-baseline: perf 첫 실습 (개선 전 기준선)

## 목적
`demo.c`에 일부러 넣은 느린 패턴 3가지(분기 예측 실패, 캐시 미스, 순수 연산)가
perf에서 각각 어떻게 보이는지 확인하고, 이후 개선 작업의 **비교 기준선**으로 삼는다.

## 환경
- CPU: AMD Ryzen 5 7430U (6코어/12스레드), L1d 32KB/코어, L2 512KB/코어, L3 16MB
- perf 6.12.0, gcc 14.3.1, `perf_event_paranoid=2` (유저 공간만 측정, 이벤트 이름 뒤 `:u`)
- 빌드: `gcc -O1 -g -fno-omit-frame-pointer demo.c -o demo`

## 파일 목록
| 파일 | 내용 | 만든 명령 |
|---|---|---|
| `demo.c` | 측정한 소스 (branchy에 cmov 금지 속성 있음) | - |
| `demo_cmov.c` | 비교용: 속성을 뺀 버전 (gcc가 cmov로 바꿈) | - |
| `stat.txt` | demo 하드웨어 카운터 통계 (3회 평균) | `perf stat -r 3 -e cycles,instructions,cache-misses,cache-references,branches,branch-misses ./demo` |
| `stat_cmov.txt` | demo_cmov 통계 (3회 평균) | `gcc -O1 -g -fno-omit-frame-pointer demo_cmov.c -o demo_cmov` 후 위와 같은 `perf stat`을 `./results/2026-10-15_demo-baseline/demo_cmov`로 실행 (바이너리는 지움) |
| `perf.data` | 샘플링 원본 데이터 (다시 분석할 수 있음) | `perf record -g -o perf.data ./demo` |
| `report.txt` | 함수별 CPU 시간 비율 | `perf report -i perf.data --stdio --no-children` |
| `annotate_branchy.txt` | branchy 명령어별 샘플 | `perf annotate -i perf.data branchy --stdio` |
| `annotate_sum_cols.txt` | sum_cols 명령어별 샘플 | `perf annotate -i perf.data sum_cols --stdio` |
| `annotate_compute.txt` | compute 명령어별 샘플 (나중에 추가, 빌드 ID `22a48692…`가 같은 바이너리로 확인) | `perf annotate -i perf.data compute --stdio` |

다시 보기: `perf report -i perf.data` (TUI, 이 폴더에서 실행)

---

## 결과 1: `perf stat` (stat.txt와 stat_cmov.txt 비교)

| 지표 | demo (분기 사용) | demo_cmov | 의미 |
|---|---|---|---|
| 실행 시간 | **0.869초** | **0.442초** | 분기 하나만 바꿔도 거의 2배 차이 |
| cycles | 3.67G | 1.84G | CPU가 쓴 클럭 수 |
| instructions | 3.60G | 3.69G | 실행한 명령어 수는 거의 같음 |
| IPC (insn per cycle) | **0.98** | **2.00** | 클럭당 처리한 명령어 수. 높을수록 좋음 |
| branch-misses | **9.35%** (8,400만 회) | **0.00%** (9천 회) | 분기 예측 실패율 |
| cache-misses | 20.8% | 20.8% | 두 버전이 같음 (sum_cols 때문) |

### 알 수 있는 것
- **명령어 수는 같은데 시간이 2배다.** 일을 더 한 게 아니라 CPU가 **놀면서 기다린 것**이다. IPC 0.98과 2.00의 차이가 이것을 보여준다.
- 원인은 **branch-misses 8,400만 회**다. 예측이 틀릴 때마다 파이프라인을 비우고 다시 시작해야 해서 약 15~20 사이클을 손해 본다. 8,400만 × 약 20 ≈ 17억 사이클이고, 실제 cycles 차이(1.83G)와 거의 맞는다.
- **branch-misses 8,406만은 "데이터의 정확히 절반"이다.** branchy는 16.7M개 × 10회 = 1.68억 번 비교한다. 값이 0~255 랜덤이라 `< 128`은 반반이고, 예측기는 동전 던지기 수준(50%)으로 틀린다. 1.68억 × 0.5 = 8,390만으로 측정값과 거의 같다. 다른 루프의 분기는 거의 다 맞힌다는 뜻이기도 하다.
- **cmov 버전에서는 branches 자체가 2.5억 개 적다.** gcc가 `if`를 조건부 이동(cmov) 명령으로 바꿔서 분기가 아예 사라졌다. 그래서 예측이 틀릴 일도 없다. 숫자도 맞다. 원소마다 `jg` 1개(1.68억)와 조건이 참일 때만 지나가는 `jmp` 1개(0.84억)가 없어져서 합계 2.52억이다. 측정한 차이는 2.50억이다(annotate_branchy.txt의 어셈블리 참고).
- cache-misses 20%는 두 버전에서 같으니 branchy와는 상관이 없다. 원인은 sum_cols로 **추정**된다(아래). 다만 이 측정만으로 증명된 것은 아니다. 이벤트를 프로그램 전체에서 셌고, `perf record`도 cycles로 샘플링했기 때문이다.
- 이벤트 옆 `(83%)`는 하드웨어 카운터가 부족해서 이벤트들을 번갈아 측정했다는 뜻이다(멀티플렉싱). 값은 추정해서 보정한 것이다. `±0.1%` 수준이라 신뢰할 만하다.

> ⚠️ 함정: 컴파일러가 분기를 cmov로 바꾸면 "분기 예측 실패" 예제가 재현되지 않는다. 처음 실행했을 때 branch-misses가 0%로 나온 이유다. 이를 막으려고 `__attribute__((optimize("no-if-conversion",...)))`를 붙였다.

---

## 결과 2: `perf report` (report.txt)

```
58.46%  branchy     ← 분기 예측 실패
24.57%  compute     ← 나눗셈 2억 회
 6.01%  __random    ← 초기화할 때 부르는 rand() (측정 대상 아님)
                      __random_r 1.56%, rand 1.02%까지 합치면 rand 관련은 약 8.6%
 5.77%  sum_cols    ← 열 방향 순회 (캐시 미스)
 1.70%  main
 0.38%  sum_rows    ← 같은 계산을 행 방향으로
```

### 알 수 있는 것
- **최적화 우선순위는 branchy (58%)다.** 여기를 고치면 효과가 가장 크다. cmov 결과로 보면 거의 0에 가깝게 줄일 수 있다.
- compute (25%)는 나눗셈(`vdivsd`)의 처리량 때문에 느리다. 근거는 결과 3의 compute annotate에 있다. 스칼라 코드로는 줄이기 어렵다.
- **sum_cols 5.77%와 sum_rows 0.38%를 비교하면, 같은 계산인데 약 15배 차이가 난다.** 메모리 접근 순서만 달라도 이렇다.
  - 배열 `a`는 2048×2048×8B = **32MB**로, L3 캐시(16MB)보다 크다.
  - 행 방향 순회: 연속된 주소를 읽어서 캐시 라인(64B) 하나로 double 8개를 쓰고, 하드웨어 프리페처도 잘 동작한다.
  - 열 방향 순회: 한 번에 16KB씩 건너뛰어서 원소마다 새 캐시 라인을 가져와야 하고, 한 열을 다 돌면 처음 읽은 라인은 이미 캐시에서 밀려나 있다.
  - ⚠️ 그런데 **"매번 DRAM까지 간다"고 단정할 수는 없다.** sum_cols는 5회 × 4M = 2,100만 번 새 라인을 읽지만, cache-misses는 프로그램 전체를 합쳐도 850만 번이다. 가능한 이유는 두 가지다. 하드웨어 프리페처가 일정 간격(stride) 접근을 일부 미리 가져왔거나, AMD에서 일반 이벤트 `cache-misses`가 L3 미스를 그대로 세는 이벤트가 아닐 수 있다. 확인하려면 sum_cols만 따로 측정하거나 `perf record -e cache-misses`를 쓰고, `perf list`로 AMD 전용 L2/L3 이벤트를 찾아본다.
- `--no-children`은 그 함수 **자신**이 쓴 시간(self)만 보여준다. 이 옵션을 빼면 자식 함수에서 쓴 시간까지 합쳐서 main이 거의 100%로 나온다.

---

## 결과 3: `perf annotate` (명령어 단위)

### branchy (annotate_branchy.txt)
```
cmpq  %rsi,%rax     ~33%   (루프 카운터 비교)
addq  %rdx,%rcx     ~32%   (if 안쪽 s += v[i])
cmpl  $0x7f,%edx    ~22%   (v[i] < 128 비교)
```
샘플이 `jg` 분기의 **바로 다음** 명령어들에 몰려 있다. perf 샘플은 실제 원인보다 1~2 명령어 뒤에 찍힌다(skid). 그러니 **v[i] < 128 분기가 원인**이라고 읽으면 된다.

### sum_cols (annotate_sum_cols.txt)
```
vaddsd (%rax),%xmm0,%xmm0     ← 메모리에서 읽어 더함
addq   $0x4000,%rax   90.89%  ← 0x4000 = 16KB씩 건너뜀
```
`addq $0x4000`에 91%가 몰린 이유도 skid 때문이다. 실제로는 바로 앞의 **메모리 로드(vaddsd)가 캐시 미스로 기다리는 시간**이다. `$0x4000`(16KB 간격)을 보면 열 방향 순회라는 것을 어셈블리에서도 확인할 수 있다.

### compute (annotate_compute.txt)
```
vcvtsi2sd %rax,%xmm3,%xmm0           ← i를 double로 변환
vdivsd    %xmm0,%xmm2,%xmm0          ← 1.0 / i
vaddsd    %xmm0,%xmm1,%xmm1   42.55% ← x += ...
addq      $0x1,%rax           57.45% ← i++
```
샘플이 vdivsd 바로 뒤의 두 명령어에 몰려 있다. 이것도 skid이고, 실제로는 **나눗셈을 기다리는 시간**이다.
- 사이클을 대략 계산해 보면, 전체 3.67G 사이클 × 24.57% ≈ 0.90G 사이클을 2억 회 반복에 썼다. 반복 1회에 **약 4.5사이클**이다.
- Zen 3의 `divsd` 성능은 지연(latency) 약 13.5사이클, 처리량(throughput) 4.5사이클당 1개다(Agner Fog 표 기준). 반복마다 나눗셈은 서로 독립이라 겹쳐서 실행되므로, **나눗셈기의 처리량이 한계**라는 것과 정확히 맞는다.
- 덧셈 `x +=`은 반복끼리 의존하지만(약 3사이클), 4.5사이클보다 짧아서 병목이 아니다. 그래서 누산기를 여러 개로 나눠도 빨라지지 않는다.
- 줄이려면 나눗셈 횟수나 비용을 줄여야 한다. 예를 들어 `-O3 -ffast-math`로 SIMD화하면 `vdivpd`로 4개씩 나눌 수 있다. 다만 덧셈 순서가 바뀌어 결과가 미세하게 달라진다.

---

## 결론과 다음 할 일
1. **branchy**: 데이터 정렬, cmov 허용, 또는 branchless 코드(`s += (v[i] < 128) * v[i]`)로 바꾼다. 예상 효과는 전체 약 50% 단축.
2. **sum_cols**: 루프 순서를 행 방향으로 바꾼다. 예상 효과는 해당 함수 약 15배.
3. **cache-misses 원인 확인**: sum_cols만 측정하거나 `perf record -e cache-misses`로 샘플링해서, 20.8%가 정말 sum_cols 때문인지 확인한다.
4. (선택) **compute**: `-ffast-math` SIMD 버전으로 나눗셈기 처리량 한계를 넘을 수 있는지 본다.
5. 고친 뒤 같은 명령으로 측정해서 `results/<날짜>_<이름>/`에 저장하고, 이 기준선과 비교한다.
