# strace & perf 사용법 정리

리눅스에서 프로그램의 시스템콜 추적과 성능 분석을 위한 두 핵심 도구의 실전 레퍼런스.

---

## 1. strace — 시스템콜 추적

### 기본 사용법

```bash
strace ./program              # 실행하면서 추적
strace -p <PID>               # 실행 중인 프로세스에 붙기
strace -o trace.log ./program # 파일로 저장
```

### 주요 옵션

| 옵션 | 설명 |
|------|------|
| `-c` | syscall별 누적 통계 (시간/횟수/에러) |
| `-f` | 자식 프로세스/스레드까지 따라감 |
| `-T` | 각 syscall에 걸린 시간 표시 (CPU 시간) |
| `-w` | 벽시계(wall clock) 시간 표시 — I/O 대기 보일 때 |
| `-t` / `-tt` / `-ttt` | 절대 타임스탬프 (초/마이크로초/유닉스) |
| `-r` | 이전 호출과의 상대 시간 |
| `-y` | fd 옆에 실제 파일/소켓 경로 표시 |
| `-yy` | `-y` + 소켓의 IP/포트까지 |
| `-e trace=...` | 특정 syscall만 (예: `read,write` 또는 `network`, `file`, `memory`) |
| `-e signal=...` | 특정 시그널만 |
| `-s N` | 문자열 인자 최대 N자 출력 (기본 32) |
| `-p PID` | 실행 중 프로세스에 attach |
| `-o FILE` | 출력 파일 |

### 워크플로우

**Step 1. 전체 요약**

```bash
strace -c -f ./program
```

`% time` 컬럼이 높은 syscall이 1순위 의심 대상.

**Step 2. 개별 호출 타이밍**

```bash
strace -w -tt -f -o trace.log ./program
```

CPU 시간(`-T`)이 아니라 벽시계(`-w`)로 봐야 I/O 블로킹이 드러남.

**Step 3. 느린 호출 추출**

```bash
# 0.1초 이상 걸린 호출만
grep -E '<0\.[1-9]|<[1-9]' trace.log

# 임계값 기반 필터
awk '/<[0-9.]+>/ { match($0, /<([0-9.]+)>/, t); if (t[1]+0 > 0.01) print }' trace.log
```

**Step 4. 의심 syscall만 좁혀보기**

```bash
strace -e trace=read,write -T -tt -y -f ./program
strace -e trace=network -tt -y ./program
strace -e trace=file -tt -y ./program
```

### 자주 쓰는 syscall 그룹

| 그룹 | 포함되는 호출 |
|------|---------------|
| `file` | open, openat, stat, access, unlink 등 |
| `network` | socket, connect, accept, send, recv 등 |
| `process` | fork, execve, wait, exit 등 |
| `signal` | kill, sigaction, sigreturn 등 |
| `memory` | mmap, munmap, brk, mprotect 등 |
| `desc` | fd 관련 (read, write, close, dup 등) |

### 한계

- ptrace 기반이라 대상 프로그램이 **수배~수십 배 느려짐**
- 절대 시간보다는 **상대적 비율**을 신뢰할 것
- syscall 빈도 높은 프로그램에서는 측정 자체가 결과를 왜곡


### 사용예시
`strace -fp 18779 -e trace=network,desc -s 1024 -o trace.log`
일부 소켓쓰기읽기에 recvfrom, sendto를 사용하고 있을수있음
---

## 2. perf — 성능 프로파일링

### 설치 및 권한

```bash
sudo apt install linux-tools-common linux-tools-$(uname -r)

# 일반 사용자 권한 (개발 환경)
sudo sysctl kernel.perf_event_paranoid=1
sudo sysctl kernel.kptr_restrict=0
```

### 서브커맨드 한눈에

| 명령 | 용도 |
|------|------|
| `perf stat` | 하드웨어/소프트웨어 카운터 요약 |
| `perf trace` | syscall 추적 (strace 대체) |
| `perf record` | 샘플링 데이터 수집 |
| `perf report` | 수집된 데이터 분석 (TUI) |
| `perf script` | record 데이터를 텍스트로 출력 |
| `perf top` | 실시간 프로파일링 (htop의 함수 버전) |
| `perf annotate` | 어셈블리 단위로 hot path 보기 |
| `perf list` | 사용 가능한 이벤트 목록 |

### perf stat — 카운터 요약

```bash
perf stat ./program
perf stat -r 5 ./program                # 5회 평균
perf stat -e cycles,instructions,cache-misses ./program
perf stat -p <PID> sleep 10             # 10초간 측정
perf stat -a sleep 10                   # 시스템 전체
```

핵심 지표:

| 지표 | 의미 |
|------|------|
| `IPC (insn per cycle)` | 1.0 미만이면 병목 가능성 |
| `cache-misses %` | 메모리 액세스 패턴 비효율 |
| `branch-misses %` | 분기 예측 실패 |
| `context-switches` | 스케줄링 오버헤드 |

### perf trace — strace 대체

```bash
sudo perf trace ./program                       # 기본 추적
sudo perf trace -s ./program                    # 요약 (strace -c와 같음)
sudo perf trace -e read,write ./program         # 특정 syscall만
sudo perf trace --duration 1 ./program          # 1ms 이상 걸린 호출만
sudo perf trace -p <PID>                        # attach
```

strace 대비 장점: 오버헤드 적고, `--duration`으로 느린 호출만 자동 필터링.

### perf record + report — CPU 프로파일링

```bash
# 1) 수집 (-g: 콜스택 포함)
perf record -g ./program
perf record -F 99 -g ./program                  # 99Hz로 샘플링 (가볍게)
perf record -g -p <PID> sleep 30                # 30초간 attach
perf record --call-graph dwarf ./program        # 프레임 포인터 없을 때

# 2) 분석
perf report                                     # interactive TUI
perf report --stdio                             # 텍스트 출력
perf report --sort comm,dso,symbol              # 정렬 기준
```

콜스택 정확도를 위해 컴파일 시 `-fno-omit-frame-pointer` 권장.

### perf top — 실시간 모니터링

```bash
sudo perf top                                   # 시스템 전체
sudo perf top -p <PID>                          # 특정 프로세스
sudo perf top -e cache-misses                   # 특정 이벤트
```

### Flame Graph — 시각화

```bash
git clone https://github.com/brendangregg/FlameGraph.git

perf record -F 99 -g ./program
perf script | ./FlameGraph/stackcollapse-perf.pl > out.folded
./FlameGraph/flamegraph.pl out.folded > flame.svg
```

`flame.svg`를 브라우저로 열면 콜스택별 시간 분포가 한눈에 보임. 가로폭 = 누적 시간, 위로 갈수록 호출 깊이.

---

## 3. 상황별 명령 조합

### CPU 바운드 의심

```bash
perf stat ./program                             # IPC, 캐시 미스 확인
perf record -g ./program && perf report         # 함수별 시간
```

### I/O 바운드 의심

```bash
strace -w -c -f ./program                       # 어떤 syscall이 시간 잡아먹나
strace -e trace=file -T -tt -y ./program        # 파일 I/O 패턴
sudo perf trace --duration 1 ./program          # 1ms 이상 syscall
```

### 멈춘(hang) 프로세스 분석

```bash
sudo strace -p <PID> -tt -T                     # 어떤 syscall에서 멈췄나
sudo cat /proc/<PID>/stack                      # 커널 스택
sudo cat /proc/<PID>/wchan                      # 무엇을 기다리는지
```

### 락 경합 의심

```bash
strace -e trace=futex -tt -T -p <PID>           # futex 호출 패턴
sudo perf record -e sched:sched_switch -ag sleep 5
perf report
```

### 메모리/캐시 문제

```bash
perf stat -e L1-dcache-load-misses,LLC-load-misses,dTLB-load-misses ./program
perf record -e cache-misses -g ./program
perf report
```

### 컨텍스트 스위치 폭주

```bash
perf stat -e context-switches,cpu-migrations ./program
sudo perf record -e sched:sched_switch -ag sleep 5
```

### 디스크 I/O 분석

```bash
sudo perf record -e block:block_rq_issue -ag sleep 10
perf report
```

---

## 4. 도구 선택 가이드

| 알고 싶은 것 | 추천 도구 |
|--------------|-----------|
| 어떤 syscall이 호출되는지, 인자/반환값까지 | `strace` |
| 느린 syscall만 자동으로 골라보기 | `perf trace --duration` |
| CPU 시간을 어떤 함수가 쓰는지 | `perf record -g` + flame graph |
| 캐시 미스, 분기 예측, IPC | `perf stat` |
| 운영 환경에서 실시간 모니터링 | `perf top`, `bpftrace` |
| 락 경합, 스케줄링 분석 | `perf record -e sched:*` |
| 멈춘 프로세스 진단 | `strace -p`, `/proc/PID/stack` |

### 선택 기준 요약

- **인자/반환값 가독성** 필요 → strace
- **오버헤드 최소화** + **느린 호출 필터링** → perf trace
- **함수 단위 CPU 분석** → perf record + flame graph
- **하드웨어 카운터** (캐시, 분기) → perf stat
- **운영 환경** → perf 또는 bpftrace (strace는 오버헤드 큼)

---

## 5. 대안 도구

운영 환경이거나 오버헤드가 더 낮아야 할 때:

```bash
# bpftrace - eBPF 기반, 운영 환경에 적합
bpftrace -e 'tracepoint:syscalls:sys_enter_read { @[comm] = count(); }'

# bcc 도구들
sudo opensnoop-bpfcc                            # open() 호출 추적
sudo execsnoop-bpfcc                            # exec() 호출 추적
sudo biolatency-bpfcc                           # 디스크 I/O 지연 분포
sudo tcplife-bpfcc                              # TCP 연결 수명

# ltrace - 라이브러리 함수 호출 추적 (syscall 아님)
ltrace ./program
```