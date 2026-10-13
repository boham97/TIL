# eBPF로 "IO 병목"이 진짜인지 확인하기

> 데이터 수집이 느릴 때마다 "IO 병목"이라고 하는데, 그 말이 맞는지 **직접 측정해서 확인**하려고 만든 실습입니다.

---

## 1. 먼저 개념: 택배 기사 비유

프로그램을 **택배 기사**라고 생각하면 느린 이유는 셋 중 하나입니다.

| 상황 | 기사의 상태 | 컴퓨터 용어 |
|---|---|---|
| 짐 싣는 게 오래 걸림 | 계속 **일하는 중** | **CPU 병목** (코드가 느림) |
| 창고 문이 늦게 열림 | 창고 앞에서 **서서 기다림** | **디스크 IO 병목** |
| 받는 사람이 늦게 나옴 | 고객 집 앞에서 **서서 기다림** | **네트워크 IO 병목** (상대 서버가 느림) |

"IO 병목이다"라는 말은 **"프로그램이 일은 안 하고 뭔가를 기다리고 있다"**는 뜻입니다.
그러니 확인할 질문은 이것 하나입니다.

> **프로그램이 시간을 "일하는 데" 쓰나, "기다리는 데" 쓰나? 기다린다면 무엇을?**

---

## 2. eBPF가 뭔가

- **eBPF** = 리눅스 커널 안에서 작은 감시 프로그램을 안전하게 돌리는 기술입니다.
- 비유하면 기사 몸에 붙이는 **블랙박스**입니다. 기사가 어디서 몇 초씩 서 있었는지 기록합니다.
- **프로그램을 고치거나 다시 시작할 필요가 없습니다.** 돌고 있는 프로세스에 PID만 알면 붙일 수 있습니다.
- 부담도 작습니다. `strace`는 프로그램을 수 배~수십 배 느리게 만들 수 있지만, eBPF는 보통 무시할 수준입니다.
  - 단, 0은 아닙니다. 이벤트가 아주 많이 일어나는 곳(예: 모든 함수 호출)에 붙이면 느려질 수 있으니 **측정은 짧게(15~30초)** 하세요.

### 관련 용어
| 용어 | 뜻 |
|---|---|
| **BPF / eBPF** | 커널 안 감시 프로그램 기술. 요즘은 eBPF를 그냥 BPF라고도 부름 |
| **bcc-tools** | eBPF로 만든 완성품 도구 모음. `/usr/share/bcc/tools/`에 있음 |
| **bpftrace** | 한 줄짜리 eBPF 스크립트를 직접 짜서 실행하는 도구 |
| **clang / LLVM** | C 컴파일러. bcc/bpftrace가 내부적으로 eBPF 코드를 만들 때 사용 |
| **kernel-devel** | 커널 헤더 패키지. 일부 bcc 도구(offcputime, tcplife)가 필요로 함 |
| **fsync** | "방금 쓴 데이터를 디스크에 확실히 저장해 줘"라는 요청. 느림 |
| **poll** | "이 소켓(네트워크 연결)에 데이터가 오면 깨워 줘"라는 요청 |

---

## 3. 파일 구성

```
.
├── README.md       ← 이 문서
├── CLAUDE.md       ← Claude(AI)가 다음 세션에서 읽는 메모
├── workload.py     ← 병목을 일부러 만드는 테스트 프로그램
├── monitor.sh      ← 테스트 프로그램 + eBPF 측정 + 결과 요약
├── leak.c          ← 메모리 누수를 일부러 만드는 C 프로그램 (7장)
├── leak.sh         ← leak.c 빌드 + 메모리 증가 + memleak 측정
└── results/        ← 측정 원본 결과 (disk/, net/, cpu/, leak/)
```

### workload.py: 일부러 느린 프로그램 만들기
| 모드 | 하는 일 | 흉내 내는 상황 |
|---|---|---|
| `disk` | 4KB 쓰고 **매번 fsync** | 데이터를 한 건씩 저장하는 수집기 |
| `net` | 50ms 늦게 응답하는 로컬 서버에 순서대로 요청 | 느린 API/사이트를 호출하는 수집기 |
| `cpu` | JSON 처리 + 해시 계산만 반복 | 파싱/가공이 무거운 수집기 |
| `server` | `net` 모드용 느린 HTTP 서버 (포트 18080) | 느린 원격 서버 |

직접 실행할 수도 있습니다 (sudo 필요 없음).
```bash
python3 workload.py disk --duration 10
```

### monitor.sh: 측정하고 요약하기
```bash
./monitor.sh disk        # 기본 15초 측정
./monitor.sh net 30      # 30초 측정
./monitor.sh cpu
```
- 처음에 **sudo 비밀번호**를 물어봅니다 (eBPF 도구는 root 권한 필요).
- 아래 도구들을 **동시에** 돌리고 결과를 요약해서 보여줍니다.
- 원본은 `results/<모드>/`에 남고, 테스트용 파일(`data/`)은 끝나면 자동 삭제됩니다.

| 도구 | 보는 것 | 결과 파일 |
|---|---|---|
| **offcputime** ⭐ | 프로세스가 **어디서 몇 초 기다렸나** (블랙박스) | `offcpu.folded` |
| biolatency | 디스크 요청 하나가 몇 µs 걸렸나 (분포) | `biolatency.txt` |
| xfsslower | 1ms 넘게 걸린 파일 작업 목록 | `xfsslower.txt` |
| tcplife | TCP 연결마다 몇 ms 유지됐나 | `tcplife.txt` |
| vmstat | 시스템 전체 CPU/IO 상태 (eBPF 아님) | `vmstat.txt` |

---

## 4. 실제 측정 결과 (2026-09-30, 각 15초)

| | **disk** | **net** | **cpu** |
|---|---|---|---|
| 처리량 | 약 795건/초 | 19.4건/초 | 약 29,000건/초 |
| 15초 중 **기다린 시간** (offcputime) | **14.4초** | **14.8초** | **0초** |
| 어디서 기다렸나 | `fsync` → XFS 로그 기록 대기 | `poll` → 서버 응답 대기 | 안 기다림 |
| 1ms 넘은 파일 작업 (xfsslower) | 10,857건 | 0 | 0 |
| TCP 연결 (tcplife) | 0 | 약 250개, 각 약 50ms | 0 |
| vmstat `wa` | **0%** | 0% | 0% |

### 해석
- **disk**: 15초 중 14.4초를 창고 앞(fsync)에서 기다렸습니다 → **진짜 디스크 IO 병목**.
  - 그런데 biolatency를 보면 **디스크 요청 자체는 0.016~0.063ms로 빠릅니다**. fsync 한 번은 약 1.2ms가 걸렸습니다.
  - 즉 **디스크가 느린 게 아니라 "한 건마다 fsync"하는 방식이 느린 것**입니다. 디스크를 바꿀 게 아니라 **모아서 한 번에 저장**하도록 코드를 고쳐야 합니다.
- **net**: 15초 중 14.8초를 고객 집 앞(poll)에서 기다렸습니다 → IO 병목이긴 하지만 **디스크가 아니라 상대 서버가 느림**. 연결마다 약 50ms는 테스트 서버에 넣은 지연과 정확히 일치합니다.
- **cpu**: 기다린 시간 0초. 계속 일하는 중 → 이 상황에서 "IO 병목"이라고 하면 **틀린 말**입니다.

### 가장 중요한 교훈
**vmstat의 `wa`(IO 대기 %)는 세 경우 모두 0%였습니다.** 흔히 보는 지표로는 구분이 안 됩니다.
- 코어가 많으면 희석됩니다 (12코어 중 1개가 꽉 막혀도 약 8%로 보임).
- XFS 로그 대기처럼 `wa`로 집계되지 않는 대기도 있습니다.

→ **프로세스 단위로, "어디서 기다렸는지"를 보는 offcputime이 정답을 줍니다.**

---

## 5. offcpu.folded 읽는 법

한 줄 = **"이 경로로 들어가서 총 몇 µs 기다렸다"**

```
python3;entry_SYSCALL_64_after_hwframe;do_syscall_64;__x64_sys_fsync;do_fsync;xfs_file_fsync;xfs_log_force_seq;xlog_wait_on_iclog;schedule;__schedule;finish_task_switch.isra.0 13610144
```
- `;`로 구분된 건 **함수 호출 경로**입니다. **왼쪽 = 바깥(시작), 오른쪽 = 안쪽(깊은 곳)**.
- 맨 끝 숫자 = 기다린 시간, 단위 **µs**. `13610144` = **13.6초**.

### 요령: 앞뒤는 버리고 가운데만 보기
| 부분 | 의미 |
|---|---|
| `python3` | 프로그램 이름 |
| `entry_SYSCALL_64...;do_syscall_64` | 커널에 부탁하러 들어감 (공통, **무시**) |
| **`__x64_sys_fsync`** | **무슨 부탁을 했나** ← **제일 중요** |
| `xfs_file_fsync;...;xlog_wait_on_iclog` | 어디서 막혔나 (XFS 로그 기록 대기) |
| `schedule;__schedule;finish_task_switch` | 잠들었다 깨어남 (공통, **무시**) |

→ "python3가 **fsync**를 부탁했고 XFS 로그를 기다리느라 **13.6초** 잤다"

### `__x64_sys_` 뒤 단어로 판단하기
| 보이는 단어 | 뜻 |
|---|---|
| `fsync`, `write`, `read` + `xfs`, `io_schedule` | **디스크** 대기 |
| `poll`, `select`, `epoll_wait`, `recvfrom`, `connect` | **네트워크** 응답 대기 |
| `futex` | 다른 스레드/락 대기 |
| `nanosleep`, `clock_nanosleep` | 코드에서 일부러 `sleep` |

### syscall 없이 `asm_sysvec_...`로 시작하는 줄
```
python3;asm_sysvec_reschedule_ipi;irqentry_exit_to_user_mode;schedule;... 494
```
- 스스로 기다린 게 아니라 **CPU를 다른 작업에 뺏긴 것**(새치기)입니다.
- 작으면 무시하세요. 크면 CPU가 부족하다는 뜻이고 `runqlat`로 확인합니다.

| 시작 부분 | 종류 | 의미 |
|---|---|---|
| `entry_SYSCALL...__x64_sys_XXX` | **자발적** | 프로그램이 XXX를 부탁하고 기다림 |
| `asm_sysvec_...` (인터럽트) | **비자발적** | CPU 차례를 뺏김 |

### 큰 것부터 정리해서 보기
```bash
awk '{printf "%8.2f초  ", $NF/1e6; $NF=""; sub(/.*do_syscall_64;/, ""); sub(/;schedule;.*/, ""); print}' results/disk/offcpu.folded | sort -nr | head
```

### 그림으로 보기 (플레임그래프)
```bash
git clone https://github.com/brendangregg/FlameGraph
./FlameGraph/flamegraph.pl --color=io --countname=us results/disk/offcpu.folded > disk.svg
```
브라우저로 `disk.svg`를 열면 **폭이 넓은 막대 = 오래 기다린 곳**입니다. 아래가 바깥쪽, 위가 안쪽입니다.

---

## 6. 실제 수집 작업에 적용하기

수집 작업이 **돌고 있을 때** 실행하세요.

```bash
# 1) 수집 프로세스 PID 찾기
ps -ef | grep <수집프로그램이름>

# 2) 15초 동안 "어디서 기다렸나" 기록
sudo /usr/share/bcc/tools/offcputime -K -f -p <PID> 15 > real.folded

# 3) 큰 것부터 보기
awk '{printf "%8.2f초  ", $NF/1e6; $NF=""; sub(/.*do_syscall_64;/, ""); sub(/;schedule;.*/, ""); print}' real.folded | sort -nr | head
```

### 판단 기준
| 결과 | 결론 |
|---|---|
| 15초 중 기다린 시간이 **거의 0초** | "IO 병목"은 **틀림**. CPU/코드가 느림 |
| `fsync`, `xfs`, `io_schedule`가 대부분 | **디스크 병목 맞음** → 모아서 쓰기, fsync 줄이기 검토 |
| `poll`, `tcp`, `sock`, `recv`가 대부분 | **상대 서버/네트워크가 느림** → 병렬/비동기 요청, 커넥션 재사용 검토 |
| `futex`가 대부분 | 스레드끼리 서로 기다림 (락, Python GIL 등) |

더 확인하고 싶으면 함께 돌려보세요.
```bash
sudo /usr/share/bcc/tools/biolatency -D 10 1          # 디스크 자체가 느린가
sudo /usr/share/bcc/tools/xfsslower -p <PID> 1        # 1ms 넘는 파일 작업
sudo /usr/share/bcc/tools/tcplife -p <PID>            # 연결마다 몇 ms 걸리나
```

> 💡 `monitor.sh`에서 `workload.py`를 띄우는 부분을 빼고 `PID`에 수집 프로세스 번호를 넣어도 같은 요약을 볼 수 있습니다.

---

## 7. 메모리 누수 찾기 (memleak)

### 비유: 도서관 책
| 상황 | 프로그램 | 결과 |
|---|---|---|
| 책을 빌림 | `malloc` (메모리 빌리기) | |
| 다 읽고 반납 | `free` (메모리 돌려주기) | 정상 |
| **빌리고 반납을 깜빡함** | `free`를 빼먹음 | **누수** → 빌린 책이 계속 쌓임 |

누수가 있으면 프로그램 메모리가 **조금씩 계속 늘어나다가** 결국 서버 메모리가 부족해집니다.
**memleak**은 도서관 대출 장부처럼 "누가, 어디서 빌려가서 아직 안 갚았나"를 보여줍니다.

### leak.c: 일부러 누수를 만든 프로그램
- 수집 루프 흉내: 레코드 1건마다 **4KB를 빌려서** 처리하고 반납합니다 (초당 약 1000건).
- 버그: **불량 레코드(약 13%)**이면 반납(`free`)하지 않고 그냥 넘어갑니다.

```c
static void process_record(long id)
{
    char *buf = fetch_record(id);          // 여기서 4KB 빌림 (malloc)
    if (!parse_record(buf) || id % 10 == 0) {
        dropped++;
        return;                            // 버그: free(buf)를 빼먹음
    }
    saved++;
    free(buf);                             // 정상일 때만 반납
}
```

### 실행
```bash
./leak.sh        # 기본 15초 (sudo 비밀번호 물어봄)
```
- `leak.c`를 빌드하고(`-g -fno-omit-frame-pointer`: 결과에 **함수 이름**이 나오게), 띄운 뒤 두 가지를 측정합니다.

| 측정 | 보는 것 | 결과 파일 |
|---|---|---|
| RSS (`/proc/PID/status`) | **증상**: 메모리가 계속 늘어나나 (eBPF 아님) | `results/leak/rss.txt` |
| **memleak** ⭐ | **원인**: 어디서 빌리고 안 갚았나 | `results/leak/memleak.txt` |

- `memleak -o 2000`: **2초 넘게 안 갚은 것만** 봅니다. 처리하느라 잠깐 들고 있는 정상 메모리를 빼기 위해서입니다.

### 실제 측정 결과 (2026-09-30)

**1) 증상: 메모리가 1초마다 약 0.5MB씩 늘어남**
| 시각 | RSS(메모리 사용량) |
|---|---|
| 10:02:59 | 4.1 MB |
| 10:03:04 | 6.6 MB |
| 10:03:09 | 9.2 MB |
| → 10초 동안 | **+5.1 MB (초당 약 520KB)** |

계산과도 맞습니다: 초당 약 1000건 × 불량 13% × 4KB ≈ 초당 0.5MB.

**2) 원인: memleak 결과**
```
6860800 bytes in 1675 allocations from stack
    fetch_record+0x16 [leak]
    process_record+0x18 [leak]
    main+0x84 [leak]
    __libc_start_call_main+0x7e [libc.so.6]
```

| 부분 | 뜻 |
|---|---|
| `6860800 bytes` | 안 갚은 메모리 총 **약 6.9MB** |
| `in 1675 allocations` | **1675번** 빌려서 안 갚음 (6860800 ÷ 1675 = **4096** → 4KB 버퍼가 범인) |
| 아래 함수 목록 | **어디서 빌렸나**. **위 = 안쪽**, 아래 = 바깥 (offcputime의 `;` 순서와 **반대**) |

→ "`main` → `process_record` → **`fetch_record`**에서 빌린 4KB가 반납되지 않고 쌓이고 있다"

### 읽을 때 주의점
- memleak은 **빌린 곳**(`fetch_record`)을 알려줍니다. **반납을 빼먹은 곳**은 직접 찾아야 합니다.
  - `fetch_record`는 빌려서 돌려줄 뿐 잘못이 없습니다. 그걸 받은 **`process_record`에서 `return` 전에 `free`를 안 한 게** 진짜 버그입니다.
  - 요령: 목록에서 **내 코드 중 두 번째 줄**(`process_record`)부터 "받은 메모리를 모든 경로에서 반납하나?" 확인하기.
- `parse_record`의 256B는 매번 반납하므로 결과에 **나오지 않습니다** → 정상 할당은 걸러진다는 걸 확인할 수 있습니다.
- 함수 이름 대신 `0x4011cc` 같은 **주소만** 나오면 `-g -fno-omit-frame-pointer` 없이 빌드된 것입니다.
- memleak은 malloc/free를 **전부** 기록하므로 eBPF 중에서는 부담이 큰 편입니다. 운영 서버에서는 **짧게** 돌리세요.

### 실제 프로그램에 적용하기
```bash
# 메모리가 계속 느는지 먼저 확인 (1초마다, Ctrl+C로 종료)
while sleep 1; do grep VmRSS /proc/<PID>/status; done

# 늘어난다면 원인 찾기: 30초 동안, 5초 넘게 안 갚은 것, 큰 순서 10개
sudo /usr/share/bcc/tools/memleak -p <PID> -o 5000 -T 10 30 1
```
- 대상이 Python/Java처럼 **자체적으로 메모리를 관리하는 언어**면 결과가 인터프리터 내부 함수로만 보일 수 있습니다. 그럴 땐 언어 전용 도구(Python `tracemalloc` 등)가 더 쉽습니다. memleak은 **C/C++ 프로그램이나 C 확장 모듈**에서 가장 잘 맞습니다.

---

## 8. 문제 해결

| 증상 | 원인 / 해결 |
|---|---|
| `Unable to find kernel headers` | 실행 중인 커널과 버전이 같은 헤더가 없음 → `sudo dnf install kernel-devel-$(uname -r)` 또는 새 커널로 재부팅 |
| `sudo: 암호가 필요합니다` | eBPF 도구는 root 권한 필요. 직접 터미널에서 실행 |
| 결과가 전부 비어 있음 | 측정하는 동안 워크로드/수집 작업이 실제로 돌고 있었는지 확인 |
| memleak 결과에 주소(`0x...`)만 나옴 | `-g -fno-omit-frame-pointer`로 다시 빌드 |
| 도구 시작이 느림 | bcc는 실행할 때마다 clang으로 컴파일해서 몇 초 걸림 (정상) |

---

## 9. 더 알아보기
- 다른 유용한 bcc 도구: `execsnoop`(새 프로세스), `opensnoop`(파일 열기), `runqlat`(CPU 차례 대기), `profile`(CPU 어디서 쓰나), `tcpretrans`(네트워크 재전송)
- Brendan Gregg, *BPF Performance Tools* / https://www.brendangregg.com (이 도구들 대부분의 저자)
