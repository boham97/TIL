# glibc malloc RSS 증가 분석 (shp_snap_svr, PID 911068)

## 0. 요약

**증상**: 데몬이 기동하는 스냅서버의 RSS가 계속 증가

**최종 원인 (3중 복합)**
1. 스레드 아레나가 5개 → 7개(64MB 매핑 9개)로 **단조 증가**
2. 동적 `mmap_threshold` 래칫으로 8MiB 할당이 **전부 아레나로 유입** (`mmap count=0`)
3. 아레나마다 4.5~8.4MiB free 버퍼가 1개씩 눌러앉아 **약 50MB 중복**

**처방**
```bash
export GLIBC_TUNABLES=glibc.malloc.mmap_threshold=1048576:glibc.malloc.trim_threshold=2097152:glibc.malloc.arena_max=4
```

---

## 1. calloc 구현은 libc마다 다르다

### glibc
- 청크가 mmap에서 왔으면 커널이 이미 0으로 줬으므로 **memset 생략**
- top chunk를 내줄 때도 `MORECORE_CLEARS` 조건에 따라 생략 가능
- 그 외(재사용 청크)는 `clearsize > 9` 워드면 memset, 작으면 루프 언롤

### musl
- mmap 청크면 그대로 반환
- 일반 청크는 **워드 단위로 검사해서 이미 0인 워드는 건너뜀** (glibc보다 세밀)

### 핵심
C 표준은 이 최적화를 보장하지 않는다. PostgreSQL도 2010년 스레드에서 "libc가 똑똑하기를 기대하지만 glibc가 실제로 그러는지 확인해야 한다"고 논의했다.

참고: https://www.postgresql.org/message-id/201011300128.oAU1SuG07096%40momjian.us

---

## 2. 동적 mmap threshold 래칫 (핵심 개념)

```
초기 mmap_threshold = 128KB
     ↓
128KB < 크기 ≤ 32MB 인 블록을 free
     ↓
mmap_threshold = 그 블록 크기로 상승
trim_threshold = mmap_threshold × 2
     ↓
이후 같은 크기 할당이 mmap 대신 아레나(brk/heap)로 유입
     ↓
trim_threshold도 커져서 top trim이 거의 안 일어남
```

- 64비트 상한 `DEFAULT_MMAP_THRESHOLD_MAX` = 32MB
- **`M_TRIM_THRESHOLD`, `M_TOP_PAD`, `M_MMAP_THRESHOLD`, `M_MMAP_MAX` 중 하나라도 설정하면 동적 조정이 꺼진다**
- glibc는 이 코드를 제거하는 중 (BZ #30769, 2026)

참고
- 도입: https://sourceware.org/legacy-ml/libc-alpha/2006-03/msg00033.html
- 제거: https://patchwork.sourceware.org/project/glibc/patch/PAWPR08MB89823F2C316E4B5E20BC4EC68351A@PAWPR08MB8982.eurprd08.prod.outlook.com/

---

## 3. 스레드 아레나

### 구조
| 계층 | 범위 | 락 |
|---|---|---|
| tcache | 스레드별, 24~1032B, 크기당 7개 | 없음 |
| arena | 스레드 그룹별, bin + top chunk 보유 | 뮤텍스 |
| 커널 | brk(main만) / mmap | — |

- main arena만 `brk` 사용, 나머지는 `mmap`으로 만든 **64MB(`HEAP_MAX_SIZE`) 영역**
- 처음엔 `PROT_NONE`으로 예약, 필요할 때 `mprotect`로 RW 전환
- pmap에서 `rw---` + `-----` 쌍의 합이 정확히 65536KB면 스레드 아레나

### 새 아레나가 생기는 조건 (중요)
```c
a = get_free_list();           // ① 붙은 스레드가 0인 아레나
if (a == NULL) {
    if (n <= arena_test || n < narenas_limit)
        a = _int_new_arena(size);   // ② 신규 생성
    else
        a = reused_arena();         // ③ 한도 도달 시에만 돌려씀
}
```

세 조건이 **동시에** 충족될 때 생성:
1. 스레드의 첫 malloc (TLS에 아레나 없음)
2. `free_list`가 비어 있음
3. 아레나 수 < 한도

**경합은 조건이 아니다.** 이미 아레나를 배정받은 스레드는 락이 걸려도 갈아타지 않고 대기한다.

→ **아레나 개수 = 역대 동시 실행 스레드 수의 최대값**

### 아레나는 삭제되지 않는다
스레드 종료 시 `arena_thread_freeres`가 `free_list`에 넣을 뿐. 64MB 매핑과 bin 내용은 그대로 남는다. 동적 스레드 풀에서 피크가 갱신될 때마다 단조 증가.

### arena_test vs arena_max
- `arena_test` (64비트 기본 8): **이 개수를 넘어야 `__get_nprocs()`를 호출해 하드 리밋을 계산**. 게으른 평가 최적화
- 하드 리밋 = 코어 수 × 8 (64비트)
- **`arena_max`를 설정하면 `arena_test`는 무시됨**

참고
- `arena_get2` 구현: https://src.fedoraproject.org/rpms/glibc/blob/f16/f/glibc-arenalock.patch
- BZ #19048: https://openembedded.org/pipermail/openembedded-core/2016-January/234838.html

---

## 4. 메모리 반납 메커니즘

### free()가 자동으로 반납하는 경우 (전부 "꼭대기"만)

| 경로 | 조건 |
|---|---|
| mmap 청크 | **무조건** 즉시 `munmap` |
| main arena top | 해제 청크 ≥ 64KB(`FASTBIN_CONSOLIDATION_THRESHOLD`) **그리고** top ≥ `trim_threshold` → `systrim` |
| 스레드 arena top | 위와 동일 조건으로 `heap_trim` |

**힙 중간의 free 공간은 절대 자동 반납되지 않는다. 백그라운드 회수 스레드도 없다.**

### malloc_trim(0)
```
1. malloc_consolidate()   — fastbin 병합
2. 모든 아레나 × 모든 bin 순회
   → 청크 내부의 페이지 정렬된 완전한 페이지만 madvise(MADV_DONTNEED)
3. main arena면 systrim(pad)로 brk 축소
```

**반납 안 되는 것**
- 사용 중인 청크
- 페이지보다 작은 free 청크
- 청크 양끝 정렬 자투리
- 살아있는 객체가 하나라도 박힌 페이지 (4KB에 16B만 있어도 통째로 묶임)
- tcache 보유 청크

**주의**: 오래된 man page는 "꼭대기만 반납"이라고 잘못 적혀 있었다. glibc 2.8부터 모든 아레나의 빈 페이지를 madvise한다.

### MADV_DONTNEED vs MADV_FREE
- `DONTNEED`: 즉시 반환, **RSS 바로 감소**
- `FREE`: 게으른 반환, RSS 당장 안 줄어듦

### 반대 함정
큰 버퍼를 루프에서 malloc/free 하면 매번 madvise + page fault로 커널 오버헤드가 지배할 수 있다. 실측 사례에서 시스템 시간 57.94초 → 0.60초, 전체 28% 개선. **glibc가 인색한 건 의도된 설계.**

---

## 5. 진단 도구

### pmap
```bash
pmap -x PID          # Address/Kbytes/RSS/Dirty
pmap -X PID          # Pss, Private_Dirty, LazyFree 등
# 주의: -p 는 PID 옵션이 아니라 --show-path
```

판독
| 패턴 | 의미 |
|---|---|
| 낮은 주소의 작은 anon | main arena (brk 힙) |
| 64MB 정렬 `rw---` + `-----` 쌍 | 스레드 아레나 |
| `Kbytes ≈ RSS` | 전체가 상주 중 (반납 안 됨) |
| `Kbytes >> RSS` | 이미 반납됐거나 미사용 |

### malloc_info
```bash
gdb -p PID -batch \
  -ex 'set $f = (void *) fopen("/tmp/mi.xml","w")' \
  -ex 'print $f' \
  -ex 'call (int) malloc_info(0, $f)' \
  -ex 'call (int) fclose($f)' \
  -ex detach
```
> `fopen` 반환 타입 캐스팅과 `fclose`(버퍼 플러시) 둘 다 필수

XML 읽는 법
| 요소 | 의미 |
|---|---|
| `<heap nr="N">` | 아레나 하나 |
| `<size from/to/total/count>` | **free** 청크 (사용 중인 건 안 나옴) |
| `<unsorted>` | free 후 아직 분류 안 된 청크 |
| `<total type="fast">` | fastbin free |
| `<total type="rest">` | free 총합 + **top chunk 포함** |
| `<system type="current"/"max">` | 현재/최대 힙 크기. 같으면 한 번도 안 줄어듦 |
| `<aspace type="subheaps">` | 64MB 매핑 개수 |
| `<total type="mmap" count>` | **0이면 래칫에 걸린 것** |

계산 공식
```
top chunk = rest(size) - (sizes의 모든 total 합)     # count 차이가 1이어야 검증됨
사용 중   = system(current) - rest(size) - fast(size)
```

### mallinfo2
```c
struct mallinfo2 mi = mallinfo2();   // glibc 2.33+
// arena, uordblks, fordblks, keepcost, hblks, hblkhd
```
- `fordblks`에 **top chunk 포함** (free로 취급)
- `keepcost` = top chunk = "malloc_trim이 이상적으로 반환 가능한 최대"
- **`fordblks - keepcost` = 반환 불가능한 파편화 양**
- 구버전 `mallinfo()`는 `int` 필드라 2GB 초과 시 오버플로

### strace
```bash
strace -f -e trace=memory PID                    # brk/mmap/munmap/madvise 등 일괄
strace -f -c -e trace=futex -p PID               # 요약 (CPU 시간)
strace -f -c -w -e trace=futex -p PID            # 요약 (지연 시간) ← 락 경합은 -w 필수
timeout -s INT 10 strace ...                     # timeout은 -s INT 필수 (SIGTERM이면 요약 안 나옴)
```
- `-c`는 요약만, `-C`는 요약 + 일반 출력
- Ctrl-C는 **한 번만** 누르고 detach 완료까지 대기
- 컨테이너에서는 `--cap-add=SYS_PTRACE` 필요
- 10~100배 느려짐 → 프로덕션은 `perf trace` 또는 bpftrace

### 유용한 bpftrace
```bash
# malloc 요청 크기 분포 — "크기가 진짜 제각각인가" 확인용
bpftrace -e 'uprobe:/lib/x86_64-linux-gnu/libc.so.6:malloc { @sz = hist(arg0); }'
```

---

## 6. 실측 결과 타임라인

### 1차 측정
- 아레나 6개, RSS 131MB 중 **124MB(94%)가 아레나**
- main 힙은 292KB뿐 → 모든 할당이 워커 스레드에서 발생

### malloc_trim(0) 효과
| 아레나 | 전 | 후 | 감소 |
|---|---|---|---|
| 1c000000 | 10,712 | 2,524 | -8,188 |
| 24000000 | 29,012 | 28,960 | -52 |
| 28000000 | 29,464 | 21,220 | -8,244 |
| 2c000000 | 26,288 | 18,048 | -8,240 |
| 30000000 | 2,628 | 2,624 | -4 |
| 34000000 | 26,776 | 18,532 | -8,244 |

- **총 -32,972KB, 전부 아레나에서 나옴** (VIRT는 442,904KB 그대로 → madvise 확인)
- 감소량이 ~8MB로 균일 → 아레나마다 8MiB free 청크 1개씩

### malloc_info 1차
- **아레나 5개 전부 8,388,625바이트(8MiB+헤더) free 청크 보유**
- free 총합 43MB 중 **41MB(97%)가 이 5개**
- `<total type="mmap" count="0"/>` → **래칫 확정**
- 모든 heap이 `current == max` → 힙 축소 전무

### 부하 후 (아레나 7개, 서브힙 9개)
| heap | subheaps | system | 큰 free 청크 | top chunk |
|---|---|---|---|---|
| 0 (main) | — | 0.3MB | — | 752B |
| 1 | 1 | 22.6MB | 6.58MiB | 8.00MiB |
| 2 | 1 | 16.7MB | 8.03MiB | 32KB |
| 3 | 2 | 78.2MB | 4.51MiB | 8.00MiB |
| 4 | 2 | 89.5MB | 8.00MiB | 32KB |
| 5 | 1 | 17.9MB | 8.00MiB | 3.6KB |
| 6 | 1 | 55.9MB | 8.00MiB | 64KB |
| 7 | 1 | 60.4MB | 7.01MiB | 3KB |

- **크기가 4.5~8.4MiB로 가변** → 최초 가설("크기가 달라 재사용 실패")이 실제로 성립
- top chunk 합계 16MB, 큰 free 버퍼 합계 50MB
- heap 7만 `max 68.9MB → current 60.4MB`로 유일하게 축소

### futex 측정
| 상태 | 스레드 | calls |
|---|---|---|
| 유휴 | 7 | 1,217 |
| 부하 | 3→7 | 261,054 |

- 스레드 풀이 1 ↔ 11개로 요동, 트레이스 중에도 생성/종료 반복
- 스레드 11개일 때도 아레나는 5개에서 멈춤 → 동시 malloc 경합 창이 좁음
- futex는 앱 뮤텍스/컨디션 변수도 포함하므로 malloc 몫은 일부

### 최종 상태
```
VIRT 634MB  = 64MB 매핑 9개(576MB) + 라이브러리/스택
RES  184MB
  ├─ 큰 free 버퍼 7개 :  50MB  ← 회수 가능
  ├─ top chunk       :  16MB  ← 일부 회수 가능
  └─ 실제 사용       : 약 118MB
커밋 333MB (RW로 연 부분) → RSS 184MB, 차이 149MB는 미사용 페이지
```

---

## 7. mallinfo > RSS 인 이유

**mallinfo에 있고 RSS에 없는 것**
1. 한 번도 안 건드린 페이지 (가장 큼, 149MB)
2. 이미 `madvise(MADV_DONTNEED)`된 free 청크 (장부엔 남음)
3. 스왑 아웃된 페이지
4. 갓 mmap한 큰 청크의 미사용 부분

**RSS에만 있고 mallinfo에 없는 것**: 실행 파일, 공유 라이브러리, 스레드 스택, 앱이 직접 한 mmap (님 경우 합쳐 ~7MB)

### 커밋된 미사용 메모리의 의미
- **물리 RAM은 안 뺏는다** — 다른 프로세스가 자유롭게 사용
- 단, 익명 PRIVATE WRITABLE은 매핑 크기 전체가 `Committed_AS`에 계상
- `vm.overcommit_memory=2`면 시스템 전체 커밋 한도(swap + RAM 비율)에 걸려 **다른 프로세스 할당을 실패시킬 수 있음**
- `PROT_NONE` 영역은 커밋 장부에도 안 잡힘
- **실질적 의미: RSS 상한선 지표.** 부하 패턴이 바뀌어 미사용 영역을 건드리면 RSS가 거기까지 오른다
- cgroup `memory.max`는 실사용만 세므로, RSS 기준으로 빡빡하게 잡으면 나중에 OOM kill 위험

```bash
grep -E 'CommitLimit|Committed_AS' /proc/meminfo
cat /proc/sys/vm/overcommit_memory
```

---

## 8. 처방

### 우선순위
```
[1순위] 원인 제거
  - mmap_threshold 고정 → 래칫 차단, 8MiB가 mmap으로 직행
  - arena_max 고정      → 아레나 단조 증가 차단
  - 앱에서 8MiB 버퍼 재사용 (가장 근본적)

[2순위] 운영 중 정리
  - PSI 이벤트 또는 배치 경계에서 malloc_trim(0)

[3순위] 안전망
  - 지속 초과 시 graceful 재기동
```

### 튜너블
```bash
export GLIBC_TUNABLES=glibc.malloc.mmap_threshold=1048576:glibc.malloc.trim_threshold=2097152:glibc.malloc.arena_max=4
# 구방식
export MALLOC_MMAP_THRESHOLD_=1048576
export MALLOC_TRIM_THRESHOLD_=2097152
export MALLOC_ARENA_MAX=4
```

기대: **RSS 184MB → 120~130MB**

> `arena_max=1`은 금물. 전부 main arena(brk)로 몰려 중간 구멍을 못 뚫어 오히려 악화. 최소 2 이상.

### 데몬이 기동하는 프로세스에만 적용하기

**⚠️ 자기 코드에서 `setenv()`는 안 통한다.** 튜너블은 `main()` 이전 libc 초기화 때 파싱된다.

| 방법 | 재빌드 | 설명 |
|---|---|---|
| **① mallopt()** | 스냅서버만 | `main()` 맨 앞, 스레드 생성 전 |
| **② 래퍼 스크립트** | 없음 | 데몬의 실행 경로만 교체 |
| **③ 부모에서 setenv** | 데몬 | fork/exec 직전이면 자식이 읽음 |

```c
/* ① */
#include <malloc.h>
int main(int argc, char **argv) {
    mallopt(M_MMAP_THRESHOLD, 1*1024*1024);
    mallopt(M_TRIM_THRESHOLD, 2*1024*1024);
    mallopt(M_ARENA_MAX, 4);
    ...
}
```

```bash
# ② /usr/local/bin/shp_snap_svr_wrapper
#!/bin/sh
export GLIBC_TUNABLES=glibc.malloc.mmap_threshold=1048576:glibc.malloc.arena_max=4
exec /원래경로/shp_snap_svr "$@"
```

**권장**: ②로 효과 확인 → ①로 영구 반영

### 앱 수준 근본 해결
```c
/* 크기 버킷팅 또는 최대치 재사용 */
static __thread void *buf = NULL;
if (!buf) buf = malloc(8 * 1024 * 1024);
memset(buf, 0, actually_used_bytes);   // 쓰는 만큼만 0으로
```
크기가 가변인 한 어떤 튜닝을 해도 파편화가 재발한다.

### malloc_trim 주기 호출 설계
- 쿨다운 + 히스테리시스 (상한/하한 분리)
- 배치 경계·유휴 진입 시점에만 (전역 연산이라 지연 예측 불가)
- RSS 측정은 `/proc/self/statm` (smaps_rollup은 비쌈)
- 반환값 0이거나 효과 없으면 몇 회 후 포기

### 검증
```bash
# 적용 후
grep 'type="mmap"' /tmp/mi_after.xml    # count가 0이 아니어야 성공
grep -c '<heap nr=' /tmp/mi_after.xml   # 아레나가 고정되는지
pmap -x $(pgrep shp_snap_svr) | tail -1
```

---

## 9. 부수적으로 발견된 것

- **OpenSSL 3중 로드**: `libcrypto.so.6`(구), `libcrypto.so.10`(1.0.x), `libcrypto.so.3.0.7`(3.0)가 한 프로세스에 동시 로드. 메모리 문제와 직결은 아니나 심볼 충돌 위험
- 스레드 스택 5MB × 8개, RSS는 16KB씩이라 무해
- `ldd --version` 확인 필요 — glibc 2.23 미만이면 `free_list` 순환 버그(BZ #19048) 영향권

---

## 10. 참고 링크

**glibc 소스/패치**
- `__libc_calloc`: https://github.com/str8outtaheap/heapwn/blob/master/malloc/__libc_calloc.c
- `arena_get2` 구현: https://src.fedoraproject.org/rpms/glibc/blob/f16/f/glibc-arenalock.patch
- 동적 threshold 도입: https://sourceware.org/legacy-ml/libc-alpha/2006-03/msg00033.html
- 동적 threshold 제거 [BZ #30769]: https://patchwork.sourceware.org/project/glibc/patch/PAWPR08MB89823F2C316E4B5E20BC4EC68351A@PAWPR08MB8982.eurprd08.prod.outlook.com/
- arena 한도 축소 제안(2026): https://patchwork.sourceware.org/project/glibc/patch/PAWPR08MB89821FAE648721A1627D33C68351A@PAWPR08MB8982.eurprd08.prod.outlook.com/
- BZ #19048 (arena free_list): https://openembedded.org/pipermail/openembedded-core/2016-January/234838.html
- BZ #11770 (HEAP_MAX_SIZE 64MB): https://www.sourceware.org/ml/glibc-bugs/2010-06/msg00139.html
- BZ #14827 (작은 청크만 쓰면 반납 안 됨): https://sourceware.org/bugzilla/show_bug.cgi?id=14827
- heap_trim top chunk 기준 통일: https://www.sourceware.org/ml/glibc-cvs/2015-q4/msg00035.html
- free()가 내부 청크를 madvise 안 하는 문제: https://patchwork.sourceware.org/project/glibc/patch/b8462f215aa1e4f7ad5adbba9337d57cc3ee4a7b.1776760573.git.xavier.roche@algolia.com/

**man page**
- Memory Allocation Tunables: https://sourceware.org/glibc/manual/latest/html_node/Memory-Allocation-Tunables.html
- `mallopt(3)`: https://www.mankier.com/3/mallopt
- `mallinfo2(3)`: https://www.man7.org/linux/man-pages/man3/mallinfo2.3.html
- `malloc_info(3)`: https://man7.org/linux/man-pages/man3/malloc_info.3.html
- `malloc_trim(3)`: https://www.mankier.com/3/malloc_trim
- `pmap(1)`: https://man7.org/linux/man-pages/man1/pmap.1.html
- `proc(5)`: https://man7.org/linux/man-pages/man5/proc.5.html
- Overcommit Accounting: https://android.googlesource.com/kernel/common/+/0c0d417747d8e289b2808c3fca32c845f82fd5e5/Documentation/vm/overcommit-accounting.rst
- Yama ptrace_scope: https://www.kernel.org/doc/html/latest/admin-guide/LSM/Yama.html

**기타**
- systemd MEMORY_PRESSURE: https://systemd.io/MEMORY_PRESSURE
- `sd_event_add_memory_pressure(3)`: https://www.man7.org/linux/man-pages/man3/sd_event_add_memory_pressure.3.html
- PostgreSQL calloc 논의(2010): https://www.postgresql.org/message-id/201011300128.oAU1SuG07096%40momjian.us
- PostgreSQL threshold 실측: https://www.postgresql.org/message-id/3424675.QJadu78ljV%40aivenlaptop
- musl calloc.c: https://git.etalabs.net/cgit/musl/tree/src/malloc/calloc.c?h=v0.9.15
- LPC "Limitations of malloc": https://lpc.events/event/11/contributions/1006/attachments/857/1626/limitations_malloc.pdf

---

## 부록: 곁다리 메모

**bash 산술 연산**: `$((2 - 1))` (이중 괄호). `$( )`는 명령 치환. 정수만 지원, 실수는 `bc`/`awk`. `set -e`에서 `(( x++ ))`는 결과가 0이면 종료 코드 1이라 스크립트가 죽을 수 있음.

**`$'\343\204\267'`**: 8진수 UTF-8 바이트. `ㄷ`(U+3137) = `0xE3 0x84 0xB7` = 8진수 `343 204 267`. `printf 'ㄷ' | od -b`로 확인 가능.

**MemSet**: PostgreSQL 자체 매크로(`src/include/c.h`). 정렬 + 값 0 + 작은 크기 조건일 때만 인라인 워드 루프, 아니면 libc `memset`으로 폴백. 1997년 하드웨어 기준 최적화였으나 SIMD memset이 빨라져 지금은 단계적으로 제거 중. "libc가 알아서 잘 하는데 손으로 최적화하면 손해"의 사례.
