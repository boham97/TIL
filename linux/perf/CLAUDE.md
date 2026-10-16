# perf 성능 분석 실습

Linux `perf`로 C 프로그램의 성능을 분석하는 학습용 프로젝트.

## 환경
- AMD Ryzen 5 7430U, L3 16MB / perf 6.12 / gcc 14.3
- `kernel.perf_event_paranoid=2` → 유저 공간만 측정 가능 (커널 심볼까지 보려면 `sudo sysctl kernel.perf_event_paranoid=1`)

## 파일 구조
- `demo.c`: 일부러 느린 패턴(분기 예측 실패, 캐시 미스, 연산 부하)을 넣은 예제
- `results/<YYYY-MM-DD>_<실험이름>/`: 실험별 결과 폴더 (README.md에 해석)
- `perf_읽는법.md`: perf stat/report/annotate 출력을 읽는 법 (입문용, baseline 결과를 예로 설명)
- `CPU_개념정리.md`: 파이프라인, hazard, 분기 예측, out-of-order, 지연/처리량, rand() 의사난수, RISC/CISC (2026-10-16 실험 숫자를 예로 설명)

## 빌드
```bash
gcc -O1 -g -fno-omit-frame-pointer demo.c -o demo
```
`-g`는 소스/심볼 표시용, `-fno-omit-frame-pointer`는 `perf record -g` 콜스택용.

## 규칙: perf 결과는 반드시 파일로 남긴다
perf를 실행하면 **항상** 아래 규칙대로 저장한다. 터미널에만 출력하고 끝내지 않는다.

1. 폴더: `results/<YYYY-MM-DD>_<실험이름>/`를 만든다 (예: `2026-10-15_branchy-sorted`).
2. 원본 저장:
   - 측정한 소스 사본 (`demo.c` 등): 나중에 코드가 바뀌어도 무엇을 측정했는지 알 수 있게
   - `stat.txt`: `perf stat -r 3 -e cycles,instructions,cache-misses,cache-references,branches,branch-misses ./프로그램 2> stat.txt`
   - `perf.data`: `perf record -g -o <폴더>/perf.data ./프로그램`
   - `report.txt`: `perf report -i perf.data --stdio --no-children`
   - `annotate_<함수>.txt`: 상위 핫스팟 함수에 대해 `perf annotate -i perf.data <함수> --stdio`
3. **`README.md` 작성 (필수)**. 나중에 다시 봐도 이해할 수 있게 다음을 모두 적는다.
   - 목적: 무엇을 확인하려는 실험인지
   - 환경과 빌드 옵션
   - 파일 목록: 각 파일이 무엇이고 **어떤 명령으로 만들었는지**
   - 각 결과의 핵심 숫자와 **"알 수 있는 것"**: 숫자가 무엇을 뜻하는지, 왜 그렇게 나왔는지
   - 이전 실험과 비교 (기준선: `results/2026-10-15_demo-baseline/`)
   - 결론과 다음 할 일
4. `perf.data`는 머신과 바이너리에 의존한다. 바이너리를 다시 빌드하면 annotate가 어긋날 수 있으니 txt 출력도 꼭 남긴다.
5. 프로젝트 루트에 `perf.data`를 남기지 않는다 (결과 폴더로 옮기거나 `-o`로 지정).

## 해석할 때 주의점 (지금까지 배운 것)
- **IPC** (insn per cycle): 1 미만이면 CPU가 무언가를 기다리며 노는 중이다(메모리, 분기 예측 실패). 2 이상이면 잘 돌고 있다.
- **컴파일러가 `if`를 cmov로 바꾸면 분기 예측 실패가 사라진다.** 예제가 재현되지 않으면 `perf annotate`로 실제 어셈블리를 확인한다.
- **skid**: annotate 샘플은 실제 원인 명령어보다 1~2개 **뒤**에 찍힌다. 비싼 명령어(메모리 로드, 분기)는 핫한 줄 바로 위에서 찾는다.
- 이벤트 옆 `(83%)`는 멀티플렉싱으로 일부 시간만 측정했다는 뜻이다(값은 보정된 추정치). `-r 3`의 `±` 편차로 신뢰도를 확인한다.
- **숫자로 검산한다.** 예: branch-misses ≈ 비교 횟수 × 50%(랜덤 데이터), 반복당 사이클 = 전체 cycles × 함수 비율 ÷ 반복 횟수를 명령어 지연/처리량과 비교한다. 계산이 맞으면 해석이 맞는 것이다.
- **`cache-misses`는 AMD에서 의미가 모호하다.** baseline에서 예상 미스 2,100만 번 vs 측정 850만 번. "DRAM까지 간다" 같은 단정은 함수별로 따로 측정해 확인한 뒤에 한다.
- annotate는 상위 핫스팟 함수마다 남긴다(비율 순서대로). 나중에 하려면 `perf buildid-list -i perf.data`와 `readelf -n <바이너리>`로 빌드 ID가 같은지 먼저 확인한다.
- **최적화 플래그를 줬다고 SIMD가 되는 것은 아니다.** `-fopt-info-vec-missed`나 annotate로 실제로 됐는지 확인한다. 예: Zen 3(AVX-512 없음)에서는 `long → double` 벡터 변환(`vcvtqq2pd`)이 없어서 `long` 카운터 루프가 벡터화되지 않는다. `int`로 바꾸면 된다.
- `perf report --no-children`은 함수 자신의 시간(self)만 보여준다. 이 옵션이 없으면 자식 함수 시간까지 합쳐서 main이 거의 100%로 나온다.

## 실험 기록
| 날짜 | 폴더 | 요약 |
|---|---|---|
| 2026-10-15 | `results/2026-10-15_demo-baseline/` | 기준선. 0.87초, IPC 0.98. branchy 58%(분기 미스 9.35%), compute 25%, sum_cols는 sum_rows보다 약 15배 느림. cmov 버전은 0.44초 |
| 2026-10-16 | `results/2026-10-16_compute-4acc/` | compute를 누산기 4개로 분리. 0.870→0.867초(-0.4%), compute 비율 24.58%로 그대로. 나눗셈 1개당 4.49사이클로 divsd 처리량 한계라서 효과 없음 |
| 2026-10-16 | `results/2026-10-16_compute-simd/` | compute만 `-O3 -ffast-math` + 카운터 int로 vdivpd ymm SIMD화. 0.868→0.704초(-18.8%), compute 0.90G→0.224G 사이클(4.0배). long 카운터는 AVX-512 없으면 벡터화 안 됨 |
| 2026-10-16 | `results/2026-10-16_branchless/` | branchy를 `s += (v[i] < 128) * v[i]`로. gcc가 cmovg 생성. 0.866→0.442초(-48.9%), IPC 0.99→2.02, branch-misses 8,400만→1만. 예측 실패 1회 약 21.7사이클. 이제 compute가 49%로 1위 |
| 2026-10-16 | `results/2026-10-16_combined/` | compute-simd + branchless. 0.868→0.281초(3.09배), IPC 2.44. 예측 0.28초와 일치, cycles/instructions/branches 모두 단순 합과 맞음 (서로 간섭 없음) |
