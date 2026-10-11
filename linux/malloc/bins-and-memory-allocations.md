# Bins & Memory Allocations (한국어 번역)

> 원문: [HackTricks — Bins & Memory Allocations](https://book.hacktricks.wiki/en/binary-exploitation/libc-heap/bins-and-memory-allocations.html)
> 이 문서는 원문을 한국어로 번역한 것입니다. 코드와 소스 주석은 원문 그대로 두었으며, 원문의 교육 과정 광고(Tip) 부분은 생략했습니다. 원문에서 접혀 있거나 예제로만 표시된 부분("~ example" 등)은 붙여주신 텍스트에 내용이 없어 제목만 남겼습니다.

---

## 기본 정보

chunk를 보관하는 효율을 높이기 위해, 모든 chunk가 하나의 연결 리스트에만 들어가는 것이 아니라 여러 종류의 리스트가 있습니다. 이것이 bin이며, bin에는 5가지 종류가 있습니다. small bin 62개, large bin 63개, unsorted bin 1개, fast bin 10개, 그리고 스레드마다 tcache bin 64개입니다.[1][2]

unsorted bin, small bin, large bin 각각의 시작 주소는 같은 배열 안에 들어 있습니다. 인덱스 0은 사용되지 않고, 1은 unsorted bin, 2~64는 small bin, 65~127은 large bin입니다.[1]

## Tcache (스레드별 캐시) Bins

스레드들은 각자 자기 힙을 가지려고 하지만(Arenas and Subheaps 참고), 스레드가 많은 프로세스(예: 웹 서버)에서는 다른 스레드와 힙을 공유하게 될 가능성이 있습니다. 이 경우 주된 해결책은 락(lock)을 사용하는 것인데, 이는 스레드의 속도를 크게 떨어뜨릴 수 있습니다.[5]

그래서 tcache는 chunk를 병합하지 않는 단일 연결 리스트라는 점에서 스레드별 fast bin과 비슷합니다. 각 스레드는 단일 연결된 tcache bin을 64개 가지고 있습니다. 각 bin에는 같은 크기의 chunk가 최대 7개까지 들어갈 수 있으며, 크기 범위는 64비트 시스템에서 24~1032B, 32비트 시스템에서 12~516B입니다.[3][4]

스레드가 chunk를 해제할 때, 그 chunk가 tcache에 넣기에 너무 크지 않고 해당 tcache bin이 가득 차 있지 않다면(이미 7개가 아니라면) tcache에 들어갑니다. tcache에 들어갈 수 없다면, 해제 작업을 전역적으로 수행하기 위해 힙 락을 기다려야 합니다.[6]

chunk를 할당할 때, 필요한 크기의 빈 chunk가 tcache에 있으면 그것을 사용하고, 없으면 전역 bin에서 찾거나 새로 만들기 위해 힙 락을 기다려야 합니다.
여기에는 최적화도 있는데, 이 경우 힙 락을 잡고 있는 동안 스레드는 요청한 크기의 힙 chunk(7개)로 자기 tcache를 채워둡니다. 그래서 더 필요해지면 tcache에서 찾을 수 있습니다.[6]

### tcache chunk 예제 추가

### Tcache 구조체와 함수

다음 코드에서는 최대 bin 개수와 인덱스당 chunk 개수, double free를 막기 위해 만들어진 `tcache_entry` 구조체, 그리고 각 스레드가 bin의 각 인덱스 주소를 저장하는 데 쓰는 구조체인 `tcache_perthread_struct`를 볼 수 있습니다.[6]

- `tcache_entry` 와 `tcache_perthread_struct`

`__tcache_init` 함수는 `tcache_perthread_struct` 객체를 위한 공간을 생성하고 할당하는 함수입니다.[6]

- tcache_init 코드

### Tcache 인덱스

tcache는 크기에 따라 여러 bin을 가지며, 각 인덱스의 첫 번째 chunk를 가리키는 시작 포인터들과 인덱스별 chunk 개수는 하나의 chunk 안에 저장되어 있습니다. 이는 이 정보를 담고 있는 chunk(보통 첫 번째 chunk)를 찾으면 모든 tcache 시작 포인터와 tcache chunk 개수를 알아낼 수 있다는 뜻입니다.

## Fast bins

fast bin은 최근 해제된 chunk를 빠르게 접근할 수 있는 구조에 보관해서 작은 chunk의 메모리 할당 속도를 높이도록 설계되었습니다. 이 bin들은 후입선출(LIFO) 방식을 사용하는데, 이는 가장 최근에 해제된 chunk가 새 할당 요청이 왔을 때 가장 먼저 재사용된다는 뜻입니다. 스택(LIFO)의 맨 위에서 넣고 빼는 것이 큐(FIFO)보다 빠르기 때문에 이 동작은 속도 면에서 유리합니다.[5]

또한 fast bin은 이중 연결 리스트가 아닌 단일 연결 리스트를 사용하여 속도를 더 높입니다. fast bin의 chunk는 이웃과 병합되지 않으므로, 중간에서 제거할 수 있게 해주는 복잡한 구조가 필요 없습니다. 단일 연결 리스트가 이런 연산에는 더 단순하고 빠릅니다.[5]

기본적으로 여기서 일어나는 일은, 헤더(확인할 첫 번째 chunk를 가리키는 포인터)가 항상 해당 크기의 가장 최근에 해제된 chunk를 가리킨다는 것입니다. 따라서:

- 해당 크기의 새 chunk가 할당될 때, 헤더는 사용할 빈 chunk를 가리키고 있습니다. 이 빈 chunk는 다음에 사용할 chunk를 가리키고 있으므로, 이 주소가 헤더에 저장되어 다음 할당 때 사용 가능한 chunk를 어디서 가져올지 알 수 있습니다.
- chunk가 해제될 때, 해제된 chunk는 현재 사용 가능한 chunk의 주소를 저장하고, 새로 해제된 이 chunk의 주소가 헤더에 들어갑니다.

연결 리스트의 최대 크기는 0x80이며, 크기 0x20인 chunk는 인덱스 0에, 크기 0x30인 chunk는 인덱스 1에 들어가는 식으로 구성됩니다.

> **주의**
>
> fast bin의 chunk는 사용 가능(available)으로 설정되지 않기 때문에, 주변의 다른 빈 chunk와 병합되지 못하고 한동안 fast bin chunk로 유지됩니다.

```c
// From https://github.com/bminor/glibc/blob/a07e000e82cb71238259e674529c37c12dc7d423/malloc/malloc.c#L1711

/*
   Fastbins

    An array of lists holding recently freed small chunks.  Fastbins
    are not doubly linked.  It is faster to single-link them, and
    since chunks are never removed from the middles of these lists,
    double linking is not necessary. Also, unlike regular bins, they
    are not even processed in FIFO order (they use faster LIFO) since
    ordering doesn't much matter in the transient contexts in which
    fastbins are normally used.

    Chunks in fastbins keep their inuse bit set, so they cannot
    be consolidated with other free chunks. malloc_consolidate
    releases all chunks in fastbins and consolidates them with
    other free chunks.
 */

typedef struct malloc_chunk *mfastbinptr;
#define fastbin(ar_ptr, idx) ((ar_ptr)->fastbinsY[idx])

/* offset 2 to use otherwise unindexable first 2 bins */
#define fastbin_index(sz) \
  ((((unsigned int) (sz)) >> (SIZE_SZ == 8 ? 4 : 3)) - 2)


/* The maximum fastbin request size we support */
#define MAX_FAST_SIZE     (80 * SIZE_SZ / 4)

#define NFASTBINS  (fastbin_index (request2size (MAX_FAST_SIZE)) + 1)
```

> **소스 주석 번역 (참고용)**
>
> Fastbins: 최근 해제된 작은 chunk들을 담는 리스트의 배열. fastbin은 이중 연결되어 있지 않다. 단일 연결이 더 빠르고, 이 리스트들의 중간에서 chunk가 제거되는 일은 없으므로 이중 연결이 필요하지 않다. 또한 일반 bin과 달리 FIFO 순서로 처리되지도 않는데(더 빠른 LIFO를 사용), fastbin이 보통 쓰이는 일시적인 상황에서는 순서가 크게 중요하지 않기 때문이다.
>
> fastbin의 chunk는 inuse 비트가 켜진 상태로 유지되므로 다른 빈 chunk와 병합될 수 없다. `malloc_consolidate`가 fastbin의 모든 chunk를 풀어서 다른 빈 chunk들과 병합한다.

### fastbin chunk 예제 추가

## Unsorted bin

unsorted bin은 힙 관리자가 메모리 할당을 더 빠르게 하기 위해 사용하는 캐시입니다. 동작 방식은 다음과 같습니다. 프로그램이 chunk를 해제할 때, 그 chunk가 tcache나 fast bin에 들어갈 수 없고 top chunk와 맞닿아 있지도 않다면, 힙 관리자는 이를 곧바로 특정 small bin이나 large bin에 넣지 않습니다. 대신 먼저 이웃한 빈 chunk들과 병합하여 더 큰 빈 메모리 블록을 만들려고 시도합니다. 그런 다음 이 새 chunk를 "unsorted bin"이라는 범용 bin에 넣습니다.[5]

프로그램이 메모리를 요청하면, 힙 관리자는 unsorted bin에 충분한 크기의 chunk가 있는지 확인합니다. 찾으면 즉시 사용합니다. unsorted bin에서 적절한 chunk를 찾지 못하면, 이 리스트의 모든 chunk를 크기에 따라 small bin이나 large bin 중 해당하는 bin으로 옮깁니다.[5]

더 큰 chunk가 두 부분으로 나뉘고 나머지가 MINSIZE보다 크면, 그 나머지는 다시 unsorted bin에 들어간다는 점에 유의하십시오.[7]

즉 unsorted bin은 최근 해제된 메모리를 빠르게 재사용하고, 시간이 많이 드는 탐색과 병합의 필요를 줄여서 메모리 할당 속도를 높이는 방법입니다.[5]

> **주의**
>
> chunk들이 서로 다른 분류에 속하더라도, 사용 가능한 chunk가 다른 사용 가능한 chunk와 맞닿아 있다면(원래 서로 다른 bin에 속해 있었더라도) 병합된다는 점에 유의하십시오.

### unsorted chunk 예제 추가

## Small Bins

small bin은 large bin보다는 빠르지만 fast bin보다는 느립니다.[5]

62개의 각 bin은 같은 크기의 chunk들을 가집니다: 16, 24, … (최대 크기는 32비트에서 504바이트, 64비트에서 1024바이트). 이는 공간을 할당할 bin을 찾는 속도와, 이 리스트들에 항목을 넣고 빼는 속도를 높이는 데 도움이 됩니다.[5]

bin 인덱스에 따른 small bin의 크기는 다음과 같이 계산됩니다.

- 가장 작은 크기: 2\*4\*index (예: index 5 → 40)
- 가장 큰 크기: 2\*8\*index (예: index 5 → 80)

```c
// From https://github.com/bminor/glibc/blob/a07e000e82cb71238259e674529c37c12dc7d423/malloc/malloc.c#L1711
#define NSMALLBINS         64
#define SMALLBIN_WIDTH    MALLOC_ALIGNMENT
#define SMALLBIN_CORRECTION (MALLOC_ALIGNMENT > CHUNK_HDR_SZ)
#define MIN_LARGE_SIZE    ((NSMALLBINS - SMALLBIN_CORRECTION) * SMALLBIN_WIDTH)

#define in_smallbin_range(sz)  \
  ((unsigned long) (sz) < (unsigned long) MIN_LARGE_SIZE)

#define smallbin_index(sz) \
  ((SMALLBIN_WIDTH == 16 ? (((unsigned) (sz)) >> 4) : (((unsigned) (sz)) >> 3))\
   + SMALLBIN_CORRECTION)
```

small bin과 large bin 중 하나를 선택하는 함수:

```c
#define bin_index(sz) \
  ((in_smallbin_range (sz)) ? smallbin_index (sz) : largebin_index (sz))
```

### small chunk 예제 추가

## Large bins

고정된 크기의 chunk를 관리하는 small bin과 달리, 각 large bin은 일정 범위의 chunk 크기를 다룹니다. 이는 더 유연해서, 크기마다 별도의 bin이 없어도 시스템이 다양한 크기를 수용할 수 있게 해줍니다.[5]

메모리 할당자에서 large bin은 small bin이 끝나는 곳에서 시작합니다. large bin의 범위는 점점 커지는데, 예를 들어 첫 번째 bin은 512~576바이트의 chunk를, 다음 bin은 576~640바이트를 담당하는 식입니다. 이 패턴이 계속되어, 가장 큰 bin은 1MB를 넘는 모든 chunk를 담습니다.[5]

large bin은 할당에 가장 잘 맞는 chunk(best fit)를 찾기 위해 다양한 크기의 chunk 리스트를 정렬하고 탐색해야 하므로 small bin보다 동작이 느립니다. chunk가 large bin에 들어갈 때는 정렬되어야 하고, 메모리가 할당될 때는 시스템이 알맞은 chunk를 찾아야 합니다. 이 추가 작업 때문에 느리지만, 큰 할당은 작은 할당보다 드물기 때문에 받아들일 만한 트레이드오프입니다.[5]

구성은 다음과 같습니다.

- 64B 범위의 bin 32개 (small bin과 겹침)
- 512B 범위의 bin 16개 (small bin과 겹침)
- 4096B 범위의 bin 8개 (일부 small bin과 겹침)
- 32768B 범위의 bin 4개
- 262144B 범위의 bin 2개
- 나머지 크기를 위한 bin 1개

### Large bin 크기 코드

### large chunk 예제 추가

## Top Chunk

```c
// From https://github.com/bminor/glibc/blob/a07e000e82cb71238259e674529c37c12dc7d423/malloc/malloc.c#L1711

/*
   Top

    The top-most available chunk (i.e., the one bordering the end of
    available memory) is treated specially. It is never included in
    any bin, is used only if no other chunk is available, and is
    released back to the system if it is very large (see
    M_TRIM_THRESHOLD).  Because top initially
    points to its own bin with initial zero size, thus forcing
    extension on the first malloc request, we avoid having any special
    code in malloc to check whether it even exists yet. But we still
    need to do so when getting memory from system, so we make
    initial_top treat the bin as a legal but unusable chunk during the
    interval between initialization and the first call to
    sysmalloc. (This is somewhat delicate, since it relies on
    the 2 preceding words to be zero during this interval as well.)
 */

/* Conveniently, the unsorted bin can be used as dummy top on first call */
#define initial_top(M)              (unsorted_chunks (M))
```

> **소스 주석 번역 (참고용)**
>
> Top: 가장 꼭대기에 있는 사용 가능한 chunk(즉, 사용 가능한 메모리의 끝에 맞닿아 있는 chunk)는 특별하게 취급된다. 이 chunk는 어떤 bin에도 포함되지 않으며, 다른 사용 가능한 chunk가 없을 때만 사용되고, 매우 커지면 시스템에 반환된다(`M_TRIM_THRESHOLD` 참고). top은 처음에 초기 크기가 0인 자기 자신의 bin을 가리키므로 첫 malloc 요청에서 확장이 강제되며, 덕분에 malloc 안에 top이 존재하는지 확인하는 특별한 코드를 둘 필요가 없다. 하지만 시스템에서 메모리를 가져올 때는 여전히 이를 확인해야 하므로, 초기화와 첫 `sysmalloc` 호출 사이의 기간 동안 `initial_top`이 이 bin을 유효하지만 사용할 수 없는 chunk로 취급하도록 한다. (이 기간 동안 앞의 두 워드도 0이어야 한다는 점에 의존하므로 다소 까다로운 부분이다.)
>
> 편리하게도, 첫 호출 시에는 unsorted bin을 가짜(dummy) top으로 사용할 수 있다.

기본적으로 이것은 현재 사용 가능한 힙 전체를 담고 있는 chunk입니다. malloc이 수행될 때 사용할 수 있는 빈 chunk가 없으면, 이 top chunk가 크기를 줄이면서 필요한 공간을 내어줍니다.
top chunk를 가리키는 포인터는 `malloc_state` 구조체에 저장됩니다.[7]

또한 처음에는 unsorted chunk를 top chunk로 사용할 수 있습니다.

### Top Chunk 예제 관찰

## Last Remainder

malloc이 사용되면서 chunk가 나뉠 때(예: unsorted bin이나 top chunk에서), 나뉜 chunk의 나머지로 만들어진 chunk를 Last Remainder라고 부르며, 그 포인터는 `malloc_state` 구조체에 저장됩니다.[7]

## 할당 흐름 (Allocation Flow)

다음 문서를 확인하십시오.

- malloc & sysmalloc

## 해제 흐름 (Free Flow)

다음 문서를 확인하십시오.

- free

## 힙 함수 보안 검사 (Heap Functions Security Checks)

힙에서 많이 쓰이는 함수들이 수행하는 보안 검사는 다음 문서에서 확인하십시오.

- Heap Functions Security Checks

## 참고 자료 (References)

1. glibc malloc.c – bins 배열 구조 (소스 코드)
2. Heap Exploitation Part 1: Understanding the glibc Heap Implementation - Azeria Labs
3. glibc malloc.c – TCACHE_FILL_COUNT (tcache bin당 최대 chunk 수, 소스 코드)
4. glibc malloc.c – tcache chunk 크기 범위 정의 (소스 코드)
5. Heap Exploitation Part 2: glibc Heap – free, bins, tcache - Azeria Labs
6. Tcache - pwn/linux/glibc-heap/implementation - CTF Wiki
7. Core functions - Diving into glibc heap - Heap Exploitation (Dhaval Kapil)
