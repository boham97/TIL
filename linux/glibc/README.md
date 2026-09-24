# glibc 메모리 진단 툴킷

`shp_snap_svr` RSS 증가 조사 과정에서 만든 도구와 절차 모음.
같은 방법으로 다른 모듈도 진단할 수 있다.

## 파일

| 파일 | 용도 |
|---|---|
| `memcheck.sh` | 실행 중 프로세스의 malloc 상태 조회 / trim / XML 덤프 |
| `arena_test.c` | 스레드 아레나 증가 · 8MiB 캐시 복제 재현 테스트 |
| `run_compare.sh` | 튜너블 4개 조합 자동 비교 |
| `glibc-malloc-rss-분석.md` | 배경 이론 전체 (아레나, threshold 래칫, trim 동작) |

---

# 1. 상태 조회 — `memcheck.sh`

## 준비

```bash
chmod +x memcheck.sh
PID=$(pgrep -x shp_snap_svr | head -1)     # -x : 프로세스 이름 정확히 일치
```

> `pgrep -f`는 cmdline 전체를 뒤져서, 해당 이름을 인자로 쓰는 다른 프로세스까지 잡힌다.

## 사용법

```bash
./memcheck.sh $PID              # 스냅샷 1회
./memcheck.sh $PID idle1        # 스냅샷 + ./memcheck.csv 에 append
./memcheck.sh $PID -w 60        # 60초마다 반복
./memcheck.sh $PID -t           # malloc_trim(0) + RSS 전후 비교
./memcheck.sh $PID -x           # malloc_info XML → ./mi.xml
./memcheck.sh $PID -i           # 튜너블 적용 여부, 스레드, 아레나 매핑
./memcheck.sh $PID -s           # 전체 스레드 백트레이스 → 파일
```

## 출력 읽는 법

```
arena    = 51.5 MB   uordblks + fordblks (mmap 제외)
uordblks = 27.2 MB   앱이 쥐고 있는 것       ← 누수 지표
fordblks = 24.3 MB   free 인데 할당기가 보유  ← 튜닝 대상
keepcost =  0.1 MB   top chunk (trim 가능)
frag     = 24.2 MB   fordblks - keepcost (반환 불가 파편)
mmap     =  0.0 MB   0 = threshold 래칫 걸림 / 양수 = mmap 경로 작동
```

단위는 전부 **바이트**(스크립트가 MB로 환산). `hblks`만 개수.

---

# 2. 누수 판별

## 원리

| 지표 | 유휴 시점 비교 시 | 판정 |
|---|---|---|
| `uordblks` | 사이클마다 증가 | **누수** |
| `uordblks` | 비슷하게 유지 | 누수 아님 |
| `fordblks` | 증가 | 할당기 오버헤드 (튜닝 대상) |
| RSS | 증가 | 위 둘 중 하나의 결과 |

**반드시 유휴 시점끼리 비교한다.** 부하 중 값은 작업량이 섞여서 의미가 없다.

## 절차

```bash
# 부하 완전히 멈추고 스레드 정리 대기
grep Threads /proc/$PID/status
sleep 30

./memcheck.sh $PID idle1
# ── 부하 1 사이클 ──
# 다시 유휴 대기
./memcheck.sh $PID idle2

column -s, -t memcheck.csv
```

## 주의

- `uordblks`는 **장부**다. RSS보다 클 수도 작을 수도 있다
  - 크다 → 할당만 하고 안 건드린 페이지 (calloc + 새 mmap 페이지는 memset 생략)
  - 작다 → 코드·라이브러리·스택이 RSS에 더해짐
- 아레나 개수가 늘어도 `uordblks`는 안 늘어난다. 같이 오르면 "워커가 많아서 동시 작업량이 는 것"
- 첫 부하 후 안 내려오는 건 워밍업(커넥션 풀, OpenSSL 초기화, 캐시)일 수 있다. 2차 사이클로 확인

---

# 3. 누수 위치 찾기 — valgrind

## 실행

```bash
# 현재 인스턴스 실행 조건 확인
cat /proc/$PID/cmdline | tr '\0' ' '; echo
readlink /proc/$PID/exe
readlink /proc/$PID/cwd

# 한 줄로 실행 (백슬래시 줄바꿈 주의)
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --num-callers=30 --vgdb=yes --log-file=$PWD/vg.%p.log ./shp_snap_svr <인자>
```

데몬이 띄우는 구조면 래퍼로 바꿔치기:

```bash
mv shp_snap_svr shp_snap_svr.real
cat > shp_snap_svr <<'EOF'
#!/bin/sh
exec valgrind --leak-check=full --show-leak-kinds=all --num-callers=30 --vgdb=yes \
     --log-file=/절대경로/vg.%p.log "$(dirname "$0")/shp_snap_svr.real" "$@"
EOF
chmod +x shp_snap_svr
# 원복: mv shp_snap_svr.real shp_snap_svr
```

## 종료 없이 중간 스냅샷

```bash
vgdb leak_check full reachable any          # 기준점
# ── 부하 1 사이클 ──
vgdb leak_check full reachable increased    # 늘어난 것만
```

## 리포트 읽기

| 분류 | 의미 |
|---|---|
| `definitely lost` | **진짜 누수** ← 여기부터 |
| `indirectly lost` | 위에 딸린 것 |
| `possibly lost` | 오탐 많음 |
| `still reachable` | 종료 시 포인터 살아있음. 전역 캐시 축적일 수 있음 |

## 한계

- 10~50배 느림
- 스레드를 직렬화 → 동적 스레드 풀 타이밍 재현 안 됨
- 실행 중 프로세스에 attach 불가 (반드시 재기동)
- `kill -9` 하면 리포트 안 나옴. `kill -TERM`
- valgrind 위에서는 glibc malloc이 대체되므로 `malloc_trim`/`mallinfo2`/`malloc_info` 무의미

## 대안: heaptrack (attach 가능, 훨씬 가벼움)

```bash
sudo dnf install epel-release && sudo dnf install heaptrack
heaptrack -p $PID        # 부하 1 사이클 → Ctrl-C
heaptrack_print ./heaptrack.*.zst | grep -A25 'PEAK MEMORY CONSUMERS'
```

---

# 4. 튜닝

## 적용 방법 3가지

**코드 안의 `setenv()`는 안 통한다.** 튜너블은 `main()` 이전 libc 초기화 때 파싱된다.

| 방법 | 재빌드 | 설명 |
|---|---|---|
| ① 래퍼 스크립트 | 불필요 | 테스트·긴급 대응에 최적 |
| ② `mallopt()` | 스냅서버만 | `main()` 맨 앞, 스레드 생성 전 |
| ③ 부모에서 `setenv` | 데몬 | fork/exec 직전이면 자식이 읽음 |

```bash
# ① 래퍼
export GLIBC_TUNABLES=glibc.malloc.mmap_threshold=1048576:glibc.malloc.arena_max=4
exec /원래경로/shp_snap_svr.real "$@"
```

```c
/* ② mallopt */
int main(int argc, char **argv) {
    mallopt(M_MMAP_THRESHOLD, 1*1024*1024);
    mallopt(M_TRIM_THRESHOLD, 2*1024*1024);
    mallopt(M_ARENA_MAX, 4);
    ...
}
```

## 파라미터

| 튜너블 | 기본값 | 효과 |
|---|---|---|
| `mmap_threshold` | 128KB (동적 상승) | 고정하면 큰 할당이 mmap → free 즉시 munmap |
| `trim_threshold` | 128KB (mmap_threshold × 2로 상승) | 고정하면 top chunk 회수 재개 |
| `arena_max` | 0 = 코어 수 × 8 | 아레나 단조 증가 차단. **1은 금물**(main arena로 몰림) |

넷 중 **하나만 설정해도 동적 조정(래칫) 전체가 꺼진다.**

## 확인

```bash
tr '\0' '\n' < /proc/$PID/environ | grep -E 'GLIBC_TUNABLES|MALLOC_'
./memcheck.sh $PID -x     # mmap count > 0 이면 성공
```

---

# 5. 재현 테스트 — `arena_test.c`

실제 워크로드(4.5~8.4MiB 가변 calloc, 동적 스레드 풀)를 모사한다.

```bash
gcc -O2 -g -pthread -o arena_test arena_test.c
chmod +x run_compare.sh
./run_compare.sh 8 5      # 피크 스레드 8, 사이클 5
```

4가지 설정을 순서대로 돌려 비교한다.

### 실측 결과 (idle5 기준 RSS)

| 설정 | RSS | arena |
|---|---|---|
| ① 기본 | 51,052 KB | 49.6 MB |
| ② `mmap_threshold` 고정 | **1,872 KB** | 0.9 MB |
| ③ `arena_max=4` | 18,552 KB | 15.9 MB |
| ④ 둘 다 | **1,840 KB** | 0.5 MB |

**결론: `mmap_threshold` 고정이 결정적. `arena_max`는 부수적.**
②와 ④의 차이가 미미하므로, 락 경합 리스크를 지면서 `arena_max`를 넣을 이유가 크지 않다.

---

# 6. 실제 적용 결과

## shp_snap_svr

**문제 1 — 누수** (`shp_pack_sess_scope`, shp_snap_svr.c:2500)

```
40,016,432 bytes in 1,382 blocks are definitely lost
  at malloc
  by shp_pack_sess_scope (shp_snap_svr.c:2500)
  by sessionScopeProc (shp_snap_svr.c:1424)
  by ThreadFunc (shp_snap_svr.c:1046)
```

루프에서 `prev_hist = sess_hist`로 넘기는데 **마지막 반복분을 루프 밖에서 free하지 않음.**
호출 1회당 정확히 1블록씩 누수 → 1,382 blocks.

수정 후 `uordblks` 27MB → **0.9MB**.

> 주의: 같이 손댔던 `if (sess_cnt) free(inbuf);`의 조건을 제거하면 **이중 free로 abort**한다.
> `get_sess_snap_info`는 데이터가 있을 때만 `inbuf`를 할당하고, free 후 NULL로 되돌리지 않기 때문.
> 조건을 유지할 것.

**문제 2 — 아레나 오버헤드**

아레나마다 8MiB free 청크가 1개씩 상주. `mmap count=0` (래칫).

```bash
export GLIBC_TUNABLES=glibc.malloc.mmap_threshold=1048576
```

| | 전 | 후 |
|---|---|---|
| arena | 65.0 MB | **1.16 MB** |
| fordblks | 64.2 MB | 0.30 MB |
| RSS | 73,964 KB | **10,452 KB** |
| VIRT | 448 MB | 120 MB |
| hblks | 0 | 5 (부하 중) → 0 (종료 후) |

**RSS 86% 감소.**

## 다른 모듈 (참고 — 튜닝이 안 통하는 경우)

```
arena    473.1 MB
uordblks 422.6 MB   ← 89% 가 실제 사용 중
fordblks  50.5 MB
RSS      534.4 MB   ← uordblks + fordblks + 라이브러리
Threads   15,  아레나 19개
```

- `RSS ≈ uordblks + fordblks` → 할당한 걸 거의 다 실제로 건드리는 중
- 튜닝 여지는 죽은 아레나 5개(21MB) + top chunk 정도로 **30~50MB, 전체의 10% 미만**
- 이 경우는 **앱 자료구조 문제**. 링크드 리스트 시계열 저장이 원인

### 자료구조 개선 방향

| 방법 | 효과 |
|---|---|
| 청크 리스트 (노드 1개에 512 포인트) | 포인터·헤더 오버헤드 1/512, 파편화 급감 |
| `double` → `float`, 타임스탬프 델타 | 포인트당 24B → 4~6B |
| 다운샘플링 (오래된 구간 평균) | 기간에 비례해 절감 |
| 보존 한도(retention) | **없으면 결국 OOM** |

---

# 7. 개별 명령 (스크립트 없이)

```bash
# mallinfo2 — debuginfo 없어도 되는 우회 (구조체 반환 함수 캐스팅)
gdb -p $PID -batch \
  -ex 'set $b = (unsigned long *) malloc(80)' \
  -ex 'call ((void (*)(void *)) mallinfo2)($b)' \
  -ex 'printf "arena=%lu uordblks=%lu fordblks=%lu keepcost=%lu hblks=%lu hblkhd=%lu\n", $b[0],$b[7],$b[8],$b[9],$b[3],$b[4]' \
  -ex 'call (void) free($b)' -ex detach 2>/dev/null | grep arena=

# malloc_trim
gdb -p $PID -batch -ex 'call (int) malloc_trim(0)' -ex detach   # $1 = 1 이면 반환됨

# malloc_info XML (절대경로 필수 — 대상 프로세스가 파일을 만든다)
gdb -p $PID -batch \
  -ex "set \$f = (void *) fopen(\"$PWD/mi.xml\", \"w\")" \
  -ex 'print $f' \
  -ex 'call (int) malloc_info(0, $f)' \
  -ex 'call (int) fclose($f)' -ex detach

# 전체 스레드 백트레이스
gdb -p $PID -batch -ex 'set pagination off' -ex 'set print elements 0' \
  -ex 'set width 0' -ex 'thread apply all bt 20' -ex detach > ./bt.txt 2>&1

# 메모리 관련 syscall
strace -f -e trace=brk,mmap,munmap,madvise,mprotect -p $PID -o ./strace.log
strace -f -c -w -e trace=futex -p $PID          # 락 경합은 -w 필수 (기본은 CPU 시간)
timeout -s INT 30 strace ...                     # SIGTERM 이면 요약 안 나옴

# 매핑별 RSS
pmap -x $PID                                     # -p 는 PID 옵션 아님 (--show-path)
grep -E 'VmRSS|VmData|VmSwap|Threads' /proc/$PID/status
```

---

# 8. malloc_info XML 읽기

| 요소 | 의미 |
|---|---|
| `<heap nr="N">` | 아레나 하나. nr=0 은 main arena (brk 힙) |
| `<size from/to/total/count>` | **free** 청크. 사용 중인 건 안 나옴 |
| `<unsorted>` | free 후 아직 분류 안 된 청크 |
| `<total type="rest">` | free 총합 + **top chunk 포함** |
| `<system current/max>` | 현재/최대 힙 크기. 같으면 한 번도 안 줄어듦 |
| `<aspace subheaps>` | 64MB 매핑 개수 |
| `<total type="mmap" count>` | **0이면 래칫** |

```
top chunk = rest(size) - (sizes 의 모든 total 합)     # count 차이가 1이어야 검증됨
사용 중   = system(current) - rest - fast + mmap(size)
```

아레나 개수: `grep -c '<heap nr=' mi.xml` 에서 1 빼기 (main arena 제외).

---

# 9. 자주 걸린 함정

| 증상 | 원인 |
|---|---|
| gdb 에서 `'mallinfo2' has unknown return type` | glibc debuginfo 없음 → 함수 포인터 캐스팅 우회 사용 |
| `fopen` 으로 만든 XML 이 0바이트 | `fclose` 누락 (버퍼 미플러시) |
| XML 파일이 안 생김 | 상대경로 사용. 대상 프로세스 cwd 기준이 된다 → `$PWD` 절대경로 |
| strace 요약이 안 나옴 | `timeout`이 SIGTERM 으로 죽임 → `timeout -s INT` |
| strace 에 malloc/free 안 보임 | 시스템콜이 아니라 라이브러리 함수. `ltrace` 나 `LD_PRELOAD` 필요 |
| futex 시간이 0에 가깝게 나옴 | `-c` 는 CPU 시간. 대기 시간은 `-w` |
| bpftrace `MountNSException` | 컨테이너 격리. 호스트에서 실행하거나 `--cap-add=SYS_PTRACE` |
| valgrind `invalid option` | 프로그램 인자 누락 (valgrind 메시지는 `--log-file` 로 감) |
| `double free or corruption` | free 후 포인터를 NULL 로 안 돌려놓음 |

---

# 10. 판정 흐름 요약

```
RSS 증가 관측
   │
   ├─ ./memcheck.sh $PID 로 유휴 시점 2회 이상 측정
   │
   ├─ uordblks 가 사이클마다 증가?
   │     예 → 누수. valgrind / heaptrack 으로 위치 특정
   │     아니오 ↓
   │
   ├─ fordblks 가 arena 의 대부분?
   │     예 → 할당기 오버헤드. mmap_threshold / arena_max 튜닝
   │     아니오 ↓
   │
   └─ uordblks 가 arena 의 대부분 (RSS ≈ arena)
         → 앱이 실제로 많이 쓰는 것. 자료구조·동시성·보존정책 검토
```
