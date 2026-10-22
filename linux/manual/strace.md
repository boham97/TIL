# strace 사용 정리

리눅스에서 프로세스가 호출하는 시스템 콜과 받는 시그널을 추적하는 도구 `strace`의 옵션과 활용 예시를 정리한 문서입니다.
기준 버전: strace 7.1 (man7.org 매뉴얼)

---

## 목차

1. [기본 사용법](#1-기본-사용법)
2. [옵션 정리](#2-옵션-정리)
3. [--inject (시스템 콜 조작)](#3---inject-시스템-콜-조작)
4. [실전 분석 예시: nscd / SSSD 조회](#4-실전-분석-예시-nscd--sssd-조회)
5. [오버헤드 보정값 (-O)](#5-오버헤드-보정값--o)
6. [참고 자료](#6-참고-자료)

---

## 1. 기본 사용법

```bash
strace ./myprog                 # 명령을 실행하면서 추적
strace -p <PID>                 # 실행 중인 프로세스에 붙기 (Ctrl-C로 분리)
strace -f -tt -yy -p <PID>      # 스레드 포함, 마이크로초 시각, fd 상세 정보
strace -o trace.log ./myprog    # 결과를 파일로 저장
```

출력 한 줄은 `시스템콜(인자...) = 반환값` 형태이며, 실패 시 `-1 ENOENT (No such file or directory)`처럼 errno가 붙습니다.

---

## 2. 옵션 정리

### 일반 / 시작

| 옵션 | 설명 |
|---|---|
| `-e expr` | 추적 대상과 방식 지정, `!`로 제외 |
| `-p pid` | 실행 중인 프로세스에 붙기 (여러 PID 가능) |
| `-E var=val` / `-E var` | 환경변수 설정 / 제거 |
| `-u user` / `-u UID:GID` | 지정 사용자 권한으로 실행 (root 전용) |
| `--argv0=name` | argv[0] 변경 |

### 추적 범위

| 옵션 | 설명 |
|---|---|
| `-f` | 자식 프로세스·스레드까지 추적 |
| `-ff` | `-f`에 더해 프로세스별 파일로 저장 |
| `--output-separately` | `-o` 사용 시 프로세스별 파일 분리 |
| `-b execve` | execve 시점에 분리 |
| `-D` / `-DD` / `-DDD` | strace를 손자 프로세스로 실행, 프로세스 그룹·세션 분리 |
| `-I 1~4` | strace를 시그널로 중단할 수 있는 시점 |
| `--syscall-limit=N` | N개 잡으면 분리 |
| `--kill-on-exit` | strace 종료 시 대상도 종료 (`-p`와 병용 불가) |

### 필터링

| 옵션 | 설명 |
|---|---|
| `-e trace=집합` | 특정 콜만 추적 (`%file`, `%net`, `%process`, `%desc`, `%memory`, `%signal`, `%ipc`, `%creds`, `%%stat`, `%clock`, `/정규식` 등) |
| `-e trace-fds=집합` | 특정 fd 관련 콜만 |
| `-P path` | 특정 경로 접근 콜만 |
| `-e signal=집합` | 특정 시그널만 |
| `-e status=집합` | 결과 상태로 필터 (`successful`, `failed`, `unfinished` 등) |
| `-z` / `-Z` | 성공한 콜만 / 실패한 콜만 |

### 출력 형식

| 옵션 | 설명 |
|---|---|
| `-o file` / `-A` | 파일 출력 / 이어쓰기 |
| `-s N` | 문자열 최대 길이 (기본 32) |
| `-t` / `-tt` / `-ttt` | 시각 표시 (초 / 마이크로초 / epoch) |
| `-r` | 이전 콜과의 상대 시간 |
| `-T` | 콜 소요 시간 |
| `-y` / `-yy` | fd 경로 / fd 상세 정보 (소켓 등) |
| `-Y` | PID 옆에 명령 이름 |
| `-k` / `-kk` | 스택 트레이스 / 소스 정보 포함 |
| `--stack-trace-frame-limit=N` | 스택 프레임 수 제한 |
| `-e read=fd` / `-e write=fd` | 읽은 / 쓴 데이터 16진 덤프 |
| `-x` / `-xx` | 비ASCII 문자열 / 모든 문자열을 16진으로 |
| `--strings-in-hex` | 16진 이스케이프 방식 지정 |
| `-X raw/abbrev/verbose` | 상수 표시 방식 |
| `-e raw=집합` | 인자를 해석 없이 출력 |
| `-e abbrev=` / `-e verbose=` | 구조체 축약 / 역참조 대상 |
| `-v` | 구조체 전체 출력 |
| `-e decode-fds=` / `-e decode-pids=` | fd / PID 정보 해석 |
| `--pidns-translation` | PID 네임스페이스 변환 |
| `-e namespace=new` | 새 네임스페이스 표시 |
| `-e kvm=vcpu` | KVM vcpu 종료 사유 |
| `--secontext` | SELinux 컨텍스트 |
| `-i` / `-n` / `-N` | 명령어 포인터 / 콜 번호 / 인자 이름 |
| `-a column` | 반환값 정렬 열 |
| `-q` / `-qq` / `-qqq` | 부가 메시지 숨김 |
| `--always-show-pid` | 항상 PID 표시 |
| `--color=when` | 색상 출력 |

### 통계

| 옵션 | 설명 |
|---|---|
| `-c` / `-C` | 통계만 / 출력과 통계 함께 |
| `-S 기준` | 정렬 기준 (`time`, `calls`, `errors`, `name` 등) |
| `-U 열목록` | 표시할 열 |
| `-w` | 실제 경과 시간으로 집계 |
| `-O overhead` | 오버헤드 보정값 ([5장](#5-오버헤드-보정값--o) 참고) |

### 조작 (Tampering)

| 옵션 | 설명 |
|---|---|
| `--inject=...` | 시스템 콜 조작 ([3장](#3---inject-시스템-콜-조작) 참고) |
| `--fault=콜` | 에러 주입 단축형 (기본 ENOSYS) |

### 기타

| 옵션 | 설명 |
|---|---|
| `-d` | strace 디버그 출력 |
| `-h` | 도움말 |
| `-V` | 버전 |
| `--seccomp-bpf` | 추적 대상 콜에서만 멈춰 속도 향상 (`-f` 필요, `-p`에는 미적용) |
| `--tips` | 팁 출력 |
| `-F` | 폐지 예정 (`-f`와 같음) |

---

## 3. --inject (시스템 콜 조작)

프로그램이 시스템 콜을 호출할 때 strace가 가로채서 결과를 바꾸거나 지연·시그널을 끼워 넣는 기능입니다.
**"이런 에러가 나면 프로그램이 제대로 대처하는가"를 시험하는 장애 주입(fault injection) 테스트**에 사용합니다.

### 형식

```
--inject=콜집합[:error=errno|:retval=값][:signal=sig][:syscall=콜]
         [:delay_enter=시간][:delay_exit=시간]
         [:poke_enter=@argN=DATA][:poke_exit=@argN=DATA][:when=식]
```

`콜집합`은 `-e trace`와 같은 문법입니다 (`open`, `%file`, `/정규식` 등).

### 동작 종류

| 동작 | 의미 | 활용 예 |
|---|---|---|
| `error=ERRNO` | 콜이 실패한 것처럼 꾸밈 | 파일이 없다고 속이기 (`ENOENT`) |
| `retval=값` | 콜이 성공값을 돌려준 것처럼 꾸밈 | 실제로 실행하지 않고 0 반환 |
| `signal=SIG` | 콜에 들어갈 때 시그널 전달 | 특정 시점에 SIGSEGV 발생 |
| `syscall=콜` | 지정한 콜로 대체 (`%pure` 계열만 가능) | 기본 대체값 -1 대신 사용 |
| `delay_enter` / `delay_exit` | 콜 앞뒤에 지연 추가 | 느린 디스크·네트워크 흉내 |
| `poke_enter` / `poke_exit` | 콜 인자가 가리키는 메모리 덮어쓰기 (16진) | 읽은 데이터 바꿔치기 |
| `when=식` | 적용할 호출 순번 | 아래 표 참고 |

- `error`와 `retval`은 함께 쓸 수 없습니다.
- `error`나 `retval`을 쓰면 원래 시스템 콜은 **실제로 실행되지 않습니다**. strace가 콜 번호를 -1로 바꿔 커널이 아무것도 하지 않게 한 뒤 지정한 결과만 돌려줍니다.
- errno는 `ENOENT` 같은 이름이나 1~4095 숫자로 지정합니다.
- 시간 단위는 `s`, `ms`, `us`, `ns`이며 생략하면 마이크로초입니다.

### when 식

| 식 | 의미 |
|---|---|
| `N` | N번째 호출에만 |
| `N..M` | N번째부터 M번째까지 |
| `N+` | N번째부터 매번 |
| `N+S` | N번째, N+S번째, N+2S번째 … |
| `N..M+S` | 위와 같되 M번째까지만 |

범위는 N, S가 1~65535, M이 1~65534입니다.

### 예시

```bash
# 3번째 chdir부터 계속 "파일 없음"으로 실패시키기
strace --inject=chdir:error=ENOENT:when=3+ ./myprog

# /dev/urandom 관련 파일 콜만 실패시키기
strace -P /dev/urandom --inject=file:error=ENOENT ./myprog

# 모든 write를 0.5초씩 지연시켜 느린 디스크 흉내
strace --inject=write:delay_enter=500ms ./myprog

# 첫 번째 open만 "권한 없음"으로 실패
strace --inject=openat:error=EACCES:when=1 ./myprog

# 2번째 호출마다 read 실패 (2, 4, 6번째 …)
strace --inject=read:error=EIO:when=2+2 ./myprog

# unlink를 실제로 실행하지 않고 성공(0)으로 응답
strace --inject=unlink:retval=0 ./myprog

# 에러 주입과 시그널을 동시에
strace --inject=connect:error=ECONNREFUSED:signal=SIGUSR1 ./myprog

# SSSD 소켓 연결이 거부될 때 프로그램 동작 확인
strace -f --inject=connect:error=ECONNREFUSED ./myprog

# --fault 단축형 (기본 ENOSYS)
strace --fault=mmap:when=5 ./myprog
```

### 주의

> **운영 중인 프로세스(DB 서버 등)에 `-p`로 붙여서 쓰면 실제 장애가 납니다.**
> 쓰기 계열을 실패시키면 데이터 손상으로 이어질 수 있습니다.
> 반드시 테스트 환경이나 직접 실행하는 테스트 프로그램에만 사용하세요.

---

## 4. 실전 분석 예시: nscd / SSSD 조회

모니터링 도구를 strace로 추적했을 때 나온 로그를 분석한 사례입니다.

### 로그

```
socket(AF_UNIX, SOCK_STREAM|SOCK_CLOEXEC|SOCK_NONBLOCK, 0) = 40<UNIX:[24207407]>
connect(40, {sa_family=AF_UNIX, sun_path="/var/run/nscd/socket"}, 110) = -1 ENOENT
socket(AF_UNIX, SOCK_STREAM, 0) = 14<UNIX:[24207413]>
connect(14, {sa_family=AF_UNIX, sun_path="/var/lib/sss/pipes/nss"}, 110) = 0
sendto(14, "\24\0\0\0\1\0\0\0\0\0\0\0\0\0\0\0", 16, MSG_NOSIGNAL, NULL, 0) = 16
sendto(14, "\1\0\0\0", 4, MSG_NOSIGNAL, NULL, 0) = 4
sendto(14, "\24\0\0\0\22\0\0\0\0\0\0\0\0\0\0\0", 16, MSG_NOSIGNAL, NULL, 0) = 16
sendto(14, "\0\0\0\0", 4, MSG_NOSIGNAL, NULL, 0) = 4
```

### 해석

1. **nscd 연결 실패 (`ENOENT`)**: glibc가 사용자 조회 전에 nscd 캐시를 먼저 확인합니다. nscd가 없으면 다음 방법으로 넘어가므로 정상입니다.
2. **SSSD 연결**: `/var/lib/sss/pipes/nss`는 SSSD NSS 응답기 소켓입니다.
3. **메시지**: 헤더는 `길이(4) | 명령(4) | 상태(4) | 예약(4)` 16바이트이고, 그 뒤에 본문이 붙습니다.

| 명령 코드 | 의미 | 본문 |
|---|---|---|
| `0x01` | `SSS_GET_VERSION` | 1 (프로토콜 버전) |
| `0x12` | `SSS_NSS_GETPWUID` | 0 (UID 0 = root) |

즉 `getpwuid(0)` 호출입니다. ps, top 같은 도구가 UID를 사용자 이름으로 바꾸려고 프로세스마다 반복 호출합니다.

### root 조회가 비싼 이유

- SSSD는 기본적으로 `filter_users = root`라서 root를 응답하지 않습니다.
- `nsswitch.conf`가 `passwd: sss files` 순서이면 매번 **SSSD 캐시 미스 → 소켓 왕복 → "없음" 응답 → `/etc/passwd` 조회** 과정을 거칩니다.
- 개선: `files`를 `sss`보다 앞에 둡니다. authselect 환경에서는 `authselect create-profile`로 사용자 프로필을 만들어 수정합니다.

```bash
grep -E '^(passwd|group):' /etc/nsswitch.conf

# strace 없이 실제 비용 측정
time (for i in $(seq 1000); do getent passwd 0 >/dev/null; done)
time (for i in $(seq 1000); do getent -s files passwd 0 >/dev/null; done)
```

### 분석에 쓴 명령

```bash
strace -f -tt -yy -e trace=%net -e write=14 -p <PID>   # 소켓 정체와 송신 데이터 덤프
strace -f -k -e trace=sendto -p <PID>                  # 호출 스택 확인
ls -l /proc/<PID>/fd/14                                # fd 정체 확인
ss -xp | grep <PID>                                    # 유닉스 소켓 상대 확인
```

---

## 5. 오버헤드 보정값 (-O)

strace는 ptrace로 시스템 콜마다 프로세스를 멈췄다 재개하므로, 그 비용이 측정 시간에 섞입니다.
`-O`는 `-c` 통계에서 **호출 1회당 뺄 시간**을 직접 지정합니다.

- 기본값: strace가 휴리스틱으로 자동 추정
- 단위: `s`, `ms`, `us`, `ns` (생략 시 마이크로초)
- 시간 열과 `wall-total`, `wall-min`, `wall-max`, `wall-avg`에 적용
- `-tt`, `-T` 같은 **줄 단위 시간에는 적용되지 않음**

```bash
strace -c -O 15us ./myprog
strace -c -O 0 ./myprog          # 보정 없이 그대로 보기
```

적절한 값 찾기:

1. `time ./myprog`로 strace 없이 실행 시간 측정
2. `strace -c ./myprog`로 시스템 콜 누적 시간 확인
3. 차이를 호출 횟수로 나눈 값을 `-O`에 넣고 다시 비교

오버헤드 자체를 줄이려면 `--seccomp-bpf`를 함께 사용합니다 (`-f` 필요).

---

## 6. 참고 자료

- strace(1) 매뉴얼: https://man7.org/linux/man-pages/man1/strace.1.html
- strace 공식 홈페이지: https://strace.io/
- ptrace(2) 매뉴얼: https://man7.org/linux/man-pages/man2/ptrace.2.html
- SSSD 클라이언트 프로토콜 (`sss_cli.h`): https://github.com/SSSD/sssd/blob/master/src/sss_client/sss_cli.h
- sssd.conf(5) 매뉴얼: https://man.archlinux.org/man/sssd.conf.5
- nsswitch.conf(5) 매뉴얼: https://man7.org/linux/man-pages/man5/nsswitch.conf.5.html
- nscd(8) 매뉴얼: https://man7.org/linux/man-pages/man8/nscd.8.html
- getpwuid(3) 매뉴얼: https://man7.org/linux/man-pages/man3/getpwuid.3.html
- getent(1) 매뉴얼: https://man7.org/linux/man-pages/man1/getent.1.html
