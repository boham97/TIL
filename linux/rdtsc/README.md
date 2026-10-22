# rdtsc vs clock_gettime 벤치마크

x86 `rdtsc` 계열 명령어와 Linux `clock_gettime` 계열 시계의 **호출 비용, 지연 분포, 해상도, syscall 여부**를 비교한 실험입니다.

## 실험 환경

| 항목 | 값 |
|---|---|
| CPU | AMD Ryzen 5 5600 6-Core (12 threads) |
| OS | WSL2 (Linux 6.18.54.1-microsoft-standard-WSL2) |
| clocksource | `tsc` |
| TSC 플래그 | `constant_tsc`, `nonstop_tsc`, `rdtscp` |
| TSC 주파수 (측정값) | 3.500 GHz |
| 컴파일 | `gcc -O2` |
| 스레드 고정 | `sched_setaffinity`로 CPU0 고정 |

## 파일 구성

| 파일 | 내용 |
|---|---|
| `bench.c` | 호출 N회의 총 시간을 N으로 나눈 **호출당 비용(처리량)**. N=1천~1천만 × 20회 반복 통계 |
| `latency.c` | 연속 호출 간격(back-to-back delta)으로 본 **단일 호출 지연 분포**(p50~p99.999, max)와 `clock_getres` 해상도 |
| `vdso.c` | vDSO 경로와 `syscall()` 강제 호출 비교 (strace로 syscall 횟수 확인용) |

```bash
gcc -O2 -o bench   bench.c -lm && ./bench 20        # 인자: 반복 횟수
gcc -O2 -o latency latency.c    && ./latency         # 인자: 샘플 수 (기본 1천만)
gcc -O2 -o vdso    vdso.c       && ./vdso 3          # 1=vDSO, 2=syscall, 3=둘 다
strace -c -e trace=clock_gettime ./vdso 1            # vDSO 경로: syscall 0회
```

## 비교 대상

| 방식 | 설명 |
|---|---|
| `rdtsc` | TSC 틱 수를 읽는 CPU 명령어. 직렬화되지 않아 앞뒤 명령어와 순서가 바뀔 수 있음 |
| `lfence+rdtsc` | `lfence`로 앞선 명령어가 끝난 뒤 읽도록 보장. 측정 시작점의 정석 |
| `rdtscp` | 앞선 명령어 완료 후 읽음, CPU 번호(TSC_AUX)도 반환. 측정 끝점의 정석 |
| `CLOCK_MONOTONIC` | 부팅 후 경과 시간, NTP 속도 보정 O, 뒤로 가지 않음. **구간 측정 기본값** |
| `CLOCK_MONOTONIC_RAW` | MONOTONIC과 같되 NTP 보정 X (하드웨어 클럭 그대로) |
| `CLOCK_REALTIME` | 벽시계(Unix time). NTP step이나 수동 변경으로 **뒤로 갈 수 있음** |
| `CLOCK_MONOTONIC_COARSE` | 마지막 타이머 틱 값을 반환. TSC를 읽지 않아 빠르지만 해상도가 틱 간격 |

## 결과 1: 호출당 비용 (`bench.c`)

각 N마다 20회 반복한 값의 **중앙값** (ns/call). 괄호 안은 3.5GHz 기준 사이클 수입니다.

| 방식 | N=1천 | 1만 | 10만 | 100만 | 1천만 | 사이클 |
|---|---|---|---|---|---|---|
| `rdtsc` | 8.15 | 8.14 | 8.40 | 8.38 | 8.32 | ~29 |
| `lfence+rdtsc` | 15.52 | 15.06 | 15.29 | 15.49 | 15.52 | ~54 |
| `rdtscp` | 16.20 | 16.18 | 16.45 | 16.63 | 16.64 | ~58 |
| `CLOCK_MONOTONIC` | 19.79 | 19.78 | 20.09 | 20.05 | 20.10 | ~70 |
| `CLOCK_MONOTONIC_RAW` | 19.91 | 19.89 | 20.08 | 20.49 | 20.43 | ~71 |
| `CLOCK_REALTIME` | 19.35 | 19.33 | 20.21 | 20.08 | 20.15 | ~70 |
| `CLOCK_MONOTONIC_COARSE` | 3.83 | 3.82 | 3.87 | 3.94 | 3.93 | ~14 |

- 호출 비용은 N과 거의 무관하게 일정합니다.
- N이 작으면(1천) 인터럽트 한 번에도 흔들립니다. `rdtsc`는 N=1천에서 CV 29%, max 19.6ns였습니다. N ≥ 100만이면 CV가 대부분 약 1%로 안정됩니다.
- `rdtsc`의 약 8ns는 연속 호출이 파이프라인에서 겹친 **처리량** 값입니다. 순서를 보장하면 약 15~16ns로 2배가 됩니다.

## 결과 2: 단일 호출 지연 분포 (`latency.c`)

샘플 1천만 개 × 3회 실행 (단위 ns, 범위는 3회 실행 사이의 편차).

| 방식 | min | p50 | p99 | p99.9 | p99.99 | p99.999 | max |
|---|---|---|---|---|---|---|---|
| `rdtsc` | 0.3 | 10 | 10 | 10 | 20 | 1~6µs | 37~58µs |
| `lfence+rdtsc` | 10 | 20 | 20 | 20 | 30 | ~7µs | 52~80µs |
| `rdtscp` | 10 | 20 | 20 | 20~30 | 30 | ~7µs | 50~68µs |
| `CLOCK_MONOTONIC` | 20 | 20 | 30 | 30 | 40~1250 | 9~12µs | 53~184µs |
| `CLOCK_MONOTONIC_RAW` | 19 | 20 | 30 | 30 | 40~130 | 8~11µs | 42~64µs |
| `CLOCK_REALTIME` | 20 | 20 | 30 | 30 | 50~90 | 8~10µs | 57~99µs |
| `CLOCK_MONOTONIC_COARSE` | 0 | 0 | 0 | 0 | 0 | 0 | 4.0ms |

`clock_getres` 결과:

| 시계 | 해상도 |
|---|---|
| MONOTONIC / RAW / REALTIME | 1 ns |
| MONOTONIC_COARSE | 4 ms |

관찰한 내용:

- **TSC는 이 CPU에서 10ns 단위로 증가합니다.** 측정된 틱 간격이 35, 70, 105처럼 35의 배수였습니다(35틱 = 3.5GHz에서 10ns). AMD Zen이 TSC를 100MHz 기준 클럭에 맞춰 35씩 올리는 것으로 보입니다. `clock_gettime` 간격도 같은 이유로 10ns 단위로 끊깁니다.
- **p99.9까지는 모든 방식이 매우 안정적**입니다(10~30ns).
- **p99.99 이후의 꼬리 지연**(수 µs~수백 µs)은 타이머 인터럽트와 WSL2 호스트의 vCPU 선점 때문입니다. 어느 방식을 써도 똑같이 겪습니다.
- **COARSE는 간격의 100%가 0**입니다. 4ms 동안 같은 값을 반환하다가 한 번에 4ms가 뜁니다.
- 측정 주의: 결과를 저장하는 배열을 미리 써 두지 않으면 첫 측정에 page fault가 섞입니다. `rdtsc`의 p99.9가 1.3µs로 오염됐던 일이 있어서 `memset`으로 미리 터치하도록 했습니다.

## 결과 3: syscall 여부 (`vdso.c`)

`CLOCK_MONOTONIC` 100만 회 호출:

| 호출 방식 | 비용 | strace에 잡힌 syscall |
|---|---|---|
| `clock_gettime()` (glibc → vDSO) | 20 ns | **0회** |
| `syscall(SYS_clock_gettime, ...)` | 277 ns | 1,000,000회 |

- 일반 `clock_gettime`은 **커널 진입 없이** vDSO에서 끝납니다. 커널이 공유 페이지(vvar)에 기준값을 갱신해 두면, 유저 공간에서 `base_ns + ((rdtsc() - base_tsc) * mult) >> shift`를 계산합니다.
- syscall로 강제하면 약 14배 느립니다.
- syscall로 떨어지는 경우: clocksource가 `tsc`가 아닐 때(`hpet`, `acpi_pm`), `CLOCK_*_CPUTIME_ID`처럼 vDSO가 지원하지 않는 시계를 쓸 때, vDSO를 끈 경우.

## 결론

1. 이 환경(vDSO + invariant TSC)에서 `clock_gettime(CLOCK_MONOTONIC)`은 **약 20ns, syscall 없음**입니다. 직렬화한 `rdtsc`(약 15~16ns)와의 차이는 **5ns 남짓**입니다.
2. TSC가 10ns 단위로 증가하기 때문에, 이 CPU에서는 `rdtsc`가 해상도 면에서도 이점이 크지 않습니다.
3. **일반적인 시간 측정에는 `CLOCK_MONOTONIC`**을 쓰고, `rdtsc`/`rdtscp`는 사이클 단위 마이크로벤치마크처럼 수 ns라도 아껴야 할 때만 씁니다.
4. 날짜와 시각이 필요하면 `CLOCK_REALTIME`을 쓰되 구간 측정에는 쓰지 않습니다. `COARSE`는 ms 수준의 대략적인 시간에만 씁니다.
5. WSL2여도 중앙값~p99.9는 베어메탈 수준으로 안정적이었습니다. VM의 영향은 p99.99 이후의 꼬리 지연에서만 드러났습니다.

## 참고: 웹 핑거프린팅과 클럭 주파수 분석

- 수정 진동자의 제조 편차 때문에 기기마다 시계가 수~수십 ppm씩 빠르거나 느립니다. 이 **clock skew**를 장시간 측정하면 기기를 구분하는 지문으로 쓸 수 있습니다(Kohno et al., 2005, *Remote Physical Device Fingerprinting*).
- `CLOCK_MONOTONIC_RAW`는 이 하드웨어 오차가 보정 없이 드러나는 시계라 개념상 연결되지만, 그 자체가 탐지 도구는 아닙니다.
- 웹 JS는 OS 시계에 직접 접근하지 못하고 `Date.now()`(≈ REALTIME, ms 단위)와 `performance.now()`(≈ MONOTONIC)를 씁니다. Spectre 이후 브라우저들은 `performance.now()`를 수 µs~1ms로 반올림하고 지터를 섞어서 정밀 타이밍 공격과 핑거프린팅을 완화합니다.
