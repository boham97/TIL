# CHAPTER 1. The Process Model

## 1-1. 프로세스의 컨텍스트

> 컨텍스트 = 컨텍스트 스위칭 시 저장·복원해야 하는 프로세스의 모든 상태

| 구성요소 | 설명 |
|---|---|
| **text** | 프로그램 코드 |
| **data** | 전역 변수(global variables) |
| **user stack** | 사용자 프로그램 내 함수 호출용 |
| **kernel stack** | 시스템 콜 시 커널 내 함수 호출용 |
| **process structure** | PID, 스케줄링 우선순위, run state, 시그널 마스크 등. 커널이 항상 접근 가능하게 유지 (Linux `task_struct`) |
| **user structure (u-area)** | 실행 중일 때만 필요. 레지스터 상태, 열린 fd, 디바이스, 시스템 콜 인자·반환값. PCB 포인터 보유 |
| **register context** | 레지스터 값 + 프로그램 카운터(PC). 비실행 시 **PCB**에 저장 |
| **virtual address space** | 컨텍스트가 논리적으로 매핑되는 주소 범위. 커널 MM 서브시스템이 물리 주소로 변환 |
| **heap space** | `malloc()`으로 할당하는 사적 데이터 영역 |
| **shared memory** | `shmget(2)`, `shmdt(2)`, `shmctl(2)`로 할당, 프로세스 간 공유 |

> 책의 "가상 주소 공간 4GB 이상"은 32비트 기준. 64비트 리눅스는 사용자 공간 약 128TB.

## 1-2. Process Model OS

**정의**: 단일 스레드 프로세스를, 프로세서에서 실행되도록 스케줄링할 수 있는 실행 단위로 관리하는 커널
(↔ Thread Model: 스케줄링 단위가 스레드)

**UNIX 커널 서브시스템 6가지**
1. system call interface
2. file subsystem
3. process & memory management subsystem ← **스케줄링은 여기서**
4. I/O subsystem
5. network management subsystem
6. hardware control interface

## 1-3. 스케줄링 클래스

| 구분 | timeshare | realtime |
|---|---|---|
| 정책 | round-robin | round-robin 또는 FIFO |
| time-slice | 있음 | RR만 있음 (FIFO는 없음) |
| 우선순위 | 커널이 **동적** 조정 | **사용자가 고정** |
| 서열 | 낮음 | 높음 (가장 낮은 realtime > 모든 timeshare) |

**timeshare 동작**
- 실행하면 → 우선순위 하락
- `scheduler` 프로세스가 → 안 돌던 프로세스 우선순위 상승
- 결과: 모든 timeshare 프로세스에 CPU 시간 분배

**프로세스가 CPU를 놓는 4가지 경우**: 타임 슬라이스 소진 / 블록 / 자발적 양보 / 종료 (+ 고우선순위에 의한 선점)

## 1-4. 컨텍스트 스위치 절차

1. 현재 프로세스의 process structure를 **run queue에 우선순위 순으로 재연결**
   - 단, 블록된 경우엔 **sleep queue**로
2. 현재 프로세스의 레지스터 컨텍스트를 CPU 레지스터 → **PCB로 저장**
3. 최고 우선순위 프로세스의 process structure를 **run queue에서 제거**
4. 그 프로세스의 레지스터 컨텍스트를 PCB → **CPU 레지스터로 복원**
   - PC가 복원되므로 중단 지점부터 실행 재개

## 1-5. 프로세스 생명주기 · 모드 · 상태

### 두 가지 모드
- **user mode**: 사용자 코드(애플리케이션) 실행
- **kernel mode**: 시스템 콜 시 진입, 프로세스를 대신해 커널 코드 실행
- 커널 진입 경로: ① 시스템 콜 ② 컨텍스트 스위치 ③ 인터럽트

### 6가지 상태

| 상태 | 코드 | 설명 |
|---|---|---|
| Creation | SIDL | 커널이 자식 컨텍스트 생성 중. 부모는 Running, 자식은 Creation |
| Runnable | SRUN | 실행 준비 완료, run queue에 있음 |
| Running | SRUN | 실행 중. user/kernel 모드 모두 가능 (Runnable과 상태명 동일) |
| Sleeping | SSLEEP | `sleep()` 호출 또는 시스템 콜에서 블록 |
| Stopped | SSTOP | `SIGCONT` 시그널 대기 |
| Terminated | SZOMB | `exit()` 호출. **process structure만 남기고 모든 컨텍스트 반납**, 부모가 동기화(wait)하면 그것도 반납 |

> **한 시점에 오직 하나의 상태만** 가능. Creation/Terminated는 1회, 나머지는 몇 번이든 왕복.
> Creation → Sleeping 직행 같은 건 불가.

### 상태 전이를 일으키는 이벤트
`fork()` / 컨텍스트 스위치 / 사용자 코드 실행 / 시스템 콜 / 인터럽트 / `sleep()` / wakeup / `exit()`

## 1-6. 단일 스레드의 한계 (modifier 예제)

`modifier` 프로그램: `read_data()` → `modify_data()` → `write_data()`

- 제어 흐름이 **하나** → 명령어 한 번에 하나씩 → 계단식 순차 실행
- 어느 하나가 블록되면 → **즉시 컨텍스트 스위치, 전체 정지**
- `read_data()`가 `read()`에서 대기하는 동안 `modify_data()`는 놀고 있음

**해결 시도: 3개 프로세스로 분리 (공유 메모리/파이프 통신)**

| 얻는 것 | 잃는 것 |
|---|---|
| 하나 블록돼도 나머지 진행 | 세팅 시간 **최소 3배** |
| 멀티프로세서에서 병렬 실행 | 자원 3배 (process structure ×3, user structure ×3, 메모리 이미지 ×3) |

→ 순수 성능 이득 vs 오버헤드 부담을 따져야 함. **그래서 스레드가 필요하다.**


---

# 부록 A. AIX 매핑 — 클래스 ↔ 구조체 스케줄링 변수

## 정책 대응
| 책 | AIX |
|---|---|
| timeshare | `SCHED_OTHER` (nonfixed / variable priority) |
| realtime | `SCHED_FIFO`, `SCHED_RR` (fixed priority) |

> AIX는 스케줄링 단위가 **스레드**. 실제 변수는 `struct thread`, `struct proc`은 프로세스 단위 값 보관.
> 책 그림 1-5는 "process structure가 run queue에 매달린다"고 하지만, AIX 4 이후 실제로는 **thread structure**.

## 주요 변수
- **nice** — 기본 20 (범위 0~39), `nice`/`renice`로 조정
- **C (recent CPU usage)** — `ps -ef`의 `C` 컬럼, 실행 틱마다 +1, **최대 120**
- **pri** — `ps -l`의 `PRI` 컬럼. **값이 클수록 낮은 우선순위**
- **policy / fixed 플래그** — `setpri()`로 고정 가능

## 우선순위 계산식

```
priority = 40 (사용자 스레드 최소 우선순위) + nice + (C × R/32)
```

| 책 표현 | AIX 구현 |
|---|---|
| 실행하면 우선순위가 낮아진다 | C 증가 → 페널티 증가 → pri 값 상승 |
| scheduler가 대기 중인 것들의 우선순위를 올린다 | 매초 C를 D/32배로 감쇠 (기본 D=16 → 절반) |
| realtime은 커널이 조정 안 함 | fixed priority는 계산식 자체를 건너뜀 |

## CPU 페널티 요약
- CPU를 **최근에** 많이 쓸수록 뒤로 밀림 → CPU 독점 방지
- 영구적 아님: 초당 감쇠로 회복
- 상한: C가 120에서 멈춰 페널티는 **최대 60** (nice 최대 차이 40을 상쇄하기 충분)
- 결과: I/O 대기 많은 대화형 프로세스가 유리
- 고정 우선순위(SCHED_FIFO/RR, `setpri()`)는 페널티 없음

## 튜닝
```sh
schedo -o sched_R=<0~32>   # CPU 페널티 비율
schedo -o sched_D=<0~32>   # recent CPU usage 감쇠율
```
- 기본값 16 = 0.5 (원래 알고리즘과 동일)
- `R=0` → 페널티 항상 0, 우선순위가 nice 값만의 함수

---

# 부록 B. errno 참고 (ENOMEM vs EAGAIN)

| | EAGAIN | ENOMEM |
|---|---|---|
| 의미 | 자원/개수 한계, 재시도 필요 | 진짜 메모리 부족 |
| `fork()` | RLIMIT_NPROC, `threads-max`, `pid_max`, cgroup `pids.max` 도달 | 커널 구조체 할당 실패, init 죽은 PID 네임스페이스 |
| 기타 | 논블로킹 `read()`/`write()`, 락 경합, `getrandom()` | `malloc()`, `mmap()`, `execve()`, cgroup memory 리밋 |

> 과거 커널은 PID 공간 고갈까지 ENOMEM으로 보고했으나, POSIX에 맞춰 EAGAIN 반환으로 수정됨.

---

# 참고 링크

**Linux man pages**
- [fork(2)](https://man7.org/linux/man-pages/man2/fork.2.html)
- [errno(3)](https://man7.org/linux/man-pages/man3/errno.3.html)
- [sched(7) — 스케줄링 정책](https://man7.org/linux/man-pages/man7/sched.7.html)
- [shmget(2)](https://man7.org/linux/man-pages/man2/shmget.2.html)
- [ps(1) — 프로세스 상태 코드](https://man7.org/linux/man-pages/man1/ps.1.html)
- [pthreads(7)](https://man7.org/linux/man-pages/man7/pthreads.7.html)
- [pthread_attr_init(3) — 스레드 속성](https://man7.org/linux/man-pages/man3/pthread_attr_init.3.html)

**AIX**
- [AIX CPU 스케줄러 개요](https://www.mouritzen.dk/aix-doc/en_US/a_doc_lib/aixbman/prftungd/schedov.htm)
- [AIX 우선순위 계산 예제 (IBM)](https://www.ibm.com/docs/de/ssw_aix_72/performance/example_prior_calc.html)
- [AIX R/D 튜닝](http://ps-2.kev009.com/wisclibrary/aix52/usr/share/man/info/en_US/a_doc_lib/aixbman/prftungd/2365c68.htm)

**기타**
- [암달의 법칙](https://en.wikipedia.org/wiki/Amdahl%27s_law)
