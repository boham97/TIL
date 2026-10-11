# mallinfo 검증 테스트

`mallinfo()`를 프로세스 내부에서 호출한 값과, gdb로 외부에서 attach해서 같은
순간에 호출한 값(`mallinfo()`, `malloc_info()` XML)이 실제로 일치하는지 확인하는
테스트 하네스.

## 구성

| 파일 | 역할 |
|---|---|
| `alloc_target.c` | 테스트 대상. 스레드마다 `argv`로 받은 바이트 수만큼 malloc → barrier로 상태 동결 → 내부 `mallinfo()` 호출 후 raw write(2)로 기록 → `SIGUSR1` 올 때까지 `pause()` 대기 |
| `extract.gdb` | attach 상태에서 `malloc_info(0, ...)` (XML) 과 `mallinfo()` (직접 호출)를 실행 |
| `analyze.py` | internal(A) / gdb-call(B) / gdb-xml(C) 세 값을 비교 |
| `run_test.sh` | 전체 파이프라인 실행 스크립트 |

## 사용법

```bash
./run_test.sh <bytes_per_thread> <thread_count> [label]
# 예: ./run_test.sh 1048576 4 "1MB x 4 threads"
```

### 동작 순서
1. `alloc_target`을 백그라운드로 실행 → 스레드들이 각각 malloc
2. 모든 스레드 할당 완료 시점에 barrier로 멈추고, 그 순간 프로세스 스스로
   `mallinfo()`를 호출해 `internal_result.txt`에 기록 (A)
3. `ready` 파일에 PID를 쓰고 `pause()`로 정지 (이 구간엔 malloc/free 없음)
4. 스크립트가 `ready` 파일을 확인한 뒤 `gdb -p <PID> -x extract.gdb`로 attach
5. gdb가 같은 프로세스 안에서 `malloc_info()`(XML, C)와 `mallinfo()`(직접 호출, B) 실행
6. `SIGUSR1`을 보내 대상 프로세스를 정상 종료
7. `analyze.py`가 A/B/C를 비교 출력

### 결과 해석
- **[A vs B] 동일(정상)** — 같은 프로세스, 같은 순간에 내부/외부에서 부른
  `mallinfo()`는 바이트 단위로 완전히 같아야 정상. 다르면 타이밍이 어긋났거나 버그.
- **[A vs C] 차이** — `malloc_info()` XML은 스키마가 달라서(대형 mmap 블록은
  `<heap>`이 아니라 루트 `<total type="mmap">`에만 잡힘) 완전히 같을 필요는 없음.
  4096바이트 이상 차이나면 원인 후보(오버헤드/타이밍/mmap 임계값)를 같이 출력.

### stdio를 안 쓰는 이유
`fopen`/`fprintf`는 첫 사용 시 내부 버퍼를 malloc한다. 스냅샷을 찍은 시점과 gdb가
외부에서 다시 측정하는 시점 사이에 stdio를 쓰면 그 malloc 때문에 힙 상태가
달라져 두 값이 어긋난다(실측 확인됨: `uordblks`가 스트림 버퍼 크기만큼 증가).
그래서 스냅샷 이후 모든 파일 출력은 `open(2)`/`write(2)` raw syscall만 사용한다.

## 참고: mallinfo vs mallinfo2 (glibc)

이 호스트는 **glibc 2.28** → `mallinfo2`, `struct mallinfo2` 심볼 자체가 없음
(`nm -D libc.so.6`으로 확인, `mallinfo2`는 glibc 2.33부터 추가됨).

glibc 소스(2.23~2.40)를 직접 비교한 결과:

- **avail 계산 로직(`int_mallinfo()`) 은 2.23부터 최신까지 코드가 한 글자도
  안 바뀌었다.** `uordblks = system_mem − avail`, 여기서 `avail`은
  top chunk + fastbin + 일반 bin을 순회한 합. 모든 아레나를 순회해서 누적
  (풀 lock, trylock 아님 — 전 버전 동일).
- **glibc 2.33**: `int_mallinfo()`는 그대로 두고 `struct mallinfo2`(size_t 필드)를
  계산하는 `__libc_mallinfo2()`가 실제 작업을 하고, `__libc_mallinfo()`는 그 결과를
  필드별로 `int`로 잘라서(truncate) 반환하는 얇은 래퍼가 됨. `mallinfo`의 필드는
  32bit int라 누적값이 2GB 넘으면 오버플로/음수 래핑 가능.
- **glibc 2.26 (tcache 도입)**: `int_mallinfo()` 코드 자체는 안 바뀌었지만
  **tcache를 순회하지 않음** (최신 버전까지도 동일). `free()`한 작은 청크는
  tcache에 먼저 쌓이므로, tcache에 있는 동안은 `avail`에 안 잡혀서
  `uordblks`가 실제보다 과대평가될 수 있음. (`alloc_target.c`는 free를 안 하므로
  현재 테스트엔 영향 없음.)

## 참고: tcache란

glibc 2.26(2017)에서 추가된 **스레드별(per-thread) free 청크 캐시**. fastbin/bin과
별개의 구조로, 멀티스레드에서 malloc/free 반복 시 아레나 락 경합을 없애기 위해
도입됨.

```c
typedef struct tcache_entry {
    struct tcache_entry *next;      // 단일 연결리스트
} tcache_entry;

typedef struct tcache_perthread_struct {
    char counts[64];                // bin(size class)별 현재 개수
    tcache_entry *entries[64];      // bin(size class)별 연결리스트 head
} tcache_perthread_struct;

static __thread tcache_perthread_struct *tcache = NULL;  // 스레드마다 하나
```

- 64개 size class(`TCACHE_MAX_BINS`), 16바이트 단위. size class당 기본 최대
  **7개**(`TCACHE_FILL_COUNT`, 튜너블 `glibc.malloc.tcache_count`로 조절 가능)까지 보관.
- 캐시 대상 최대 청크 크기는 64bit 기준 약 1032바이트 — 그보다 크면 tcache 대상 아님.
- `free(p)`: 크기가 tcache 범위이고 해당 bin이 꽉 안 찼으면 tcache로, 꽉 찼으면
  기존처럼 fastbin/bin으로.
- `malloc(n)`: **tcache부터 제일 먼저 확인**(fastbin보다도 먼저), 있으면 락 없이 즉시 반환.
- `__thread`라 락이 필요 없어서 빠름. 그 대가로 **`int_mallinfo()`가 tcache를
  순회하지 않기 때문에**([참고: mallinfo vs mallinfo2](#참고-mallinfo-vs-mallinfo2-glibc) 항목
  참조) tcache에 머물러 있는 동안은 실제로는 재사용 가능한 빈 공간인데
  `uordblks`엔 여전히 "사용중"으로 잡힘.
- 보안: 락도 없고 초기엔 무결성 체크가 약해서 tcache poisoning / double-free 같은
  힙 익스플로잇의 단골 소재. glibc 2.29부터 `tcache_entry`에 `key` 필드를 추가해
  double-free 탐지 도입 (이 호스트의 glibc 2.28에는 아직 없음).

### malloc/free 진입 시 tcache가 항상 먼저 체크됨 (아레나 락 완전히 우회)

```c
__libc_malloc (size_t bytes)
{
  ...
#if USE_TCACHE
  if (tc_idx < mp_.tcache_bins && tcache && tcache->entries[tc_idx] != NULL)
    return tcache_get (tc_idx);      // ★ 여기서 바로 리턴, arena_get() 근처도 안 감
#endif
  arena_get (ar_ptr, bytes);          // tcache miss 났을 때만 도달 → 이때 아레나 락 잡음
  ...
}
```
`free()`도 동일한 순서(`_int_free()` 맨 앞에서 tcache 체크 후 바로 return). 즉 아레나별
per-thread 배정(`thread_arena`)이 있어도 malloc/free 때마다 여전히 `__libc_lock_lock`을
호출하는데, tcache가 먼저 채택되면 그 호출 자체가 스킵된다 — "경합을 줄이는" 아레나
분산과 "락 호출 자체를 없애는" tcache는 별개의 최적화.

주의: `alloc_target.c`는 malloc만 하고 free를 안 하므로(barrier 전까지) tcache가
채워질 일이 없다 — 이 테스트는 실질적으로 매번 아레나 경로를 탄다.

### tcache 범위(`tcache_bins`/`tcache_max_bytes`)는 정적 — mmap_threshold와 다름

```c
// 초기값 (프로세스 시작 시 고정)
.tcache_count = TCACHE_FILL_COUNT,                    // 7
.tcache_bins  = TCACHE_MAX_BINS,                       // 64
.tcache_max_bytes = tidx2usize (TCACHE_MAX_BINS - 1),  // 약 1032바이트

// 유일하게 바뀌는 경로: mallopt(M_TCACHE_MAX, ...) 또는
// GLIBC_TUNABLES=glibc.malloc.tcache_max:... 로 "명시적으로" 설정할 때뿐
do_set_tcache_max (size_t value) {
  mp_.tcache_max_bytes = value;
  mp_.tcache_bins = csize2tidx (request2size (value)) + 1;
}
```
힙 사용 패턴을 관찰해서 자동으로 늘리거나 줄이는 로직은 없음 — 한 번 정해지면(대개
기본값) 프로세스 종료까지 고정.

반면 `mmap_threshold`는 진짜 동적이다: `free()`에서 mmap된 큰 청크를 해제할 때마다
```c
if (!mp_.no_dyn_threshold && chunksize_nomask (p) > mp_.mmap_threshold ...)
  mp_.mmap_threshold = chunksize (p);   // 매번 자동 갱신
```
사용자가 `mallopt(M_MMAP_THRESHOLD, ...)`로 명시적으로 값을 지정하면 `no_dyn_threshold=1`
이 되어 이 자동조정이 꺼진다. tcache 범위는 이런 실행 중 자동조정 메커니즘 자체가 없다.

## 참고: 시스템 전체 가용 메모리(MemAvailable) — 커널 버전별 변화

`mallinfo`(프로세스 힙)와 달리 `/proc/meminfo`의 `MemAvailable`
(`si_mem_available()`)은 **커널 마이너 버전마다 계산식 자체가 바뀌어왔다**
(커널 소스 3.14~6.9 직접 비교, 함수 위치도 `fs/proc/meminfo.c`(inline, ~3.x)
→ `mm/page_alloc.c`(~4.x~6.1) → `mm/show_mem.c`(6.9~)로 이동):

- **v3.14** (MemAvailable 최초 도입): `available = i.freeram − wmark_low` 기반.
  + page cache 절반(또는 watermark만큼) + slab 회수가능분(동일 방식으로 cap).
- **v4.19**: 계산 기준이 `NR_FREE_PAGES − totalreserve_pages`로 변경.
  `NR_INDIRECTLY_RECLAIMABLE_BYTES`(dcache/icache 등 shrinker로 회수 가능한
  커널 메모리) 항목이 새로 추가됨 — **cap 없이 전액** 더함.
- **v5.15+**: 위 indirectly-reclaimable 항목을 slab 회수분과 합쳐서
  **함께 watermark로 cap** — v4.19보다 더 보수적인(작은) 값이 나옴. slab
  카운터도 페이지 단위(`NR_SLAB_RECLAIMABLE`)에서 바이트 단위
  (`NR_SLAB_RECLAIMABLE_B`)로 변경.

이 호스트 커널은 `4.18.0-553.el8_10` (RHEL8, Red Hat이 상위 버전 기능을
다수 백포트한 커널)이라 순정 4.18과 동일하게 동작한다고 단정할 수 없음 —
실제 확인이 필요하면 별도로 점검.
