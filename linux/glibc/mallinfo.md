# mallinfo(3) — Linux 매뉴얼 페이지 (한국어 번역)

> 원문: [mallinfo(3) — Linux manual page](https://www.man7.org/linux/man-pages/man3/mallinfo.3.html) (Linux man-pages 6.19)
> 이 문서는 원문을 한국어로 번역한 것이며, 코드와 실행 결과 출력은 원문 그대로 두었습니다.

---

## 이름 (NAME)

mallinfo, mallinfo2 - 메모리 할당 정보 얻기

## 라이브러리 (LIBRARY)

표준 C 라이브러리 (`libc`, `-lc`)

## 개요 (SYNOPSIS)

```c
#include <malloc.h>

struct mallinfo mallinfo(void);
struct mallinfo2 mallinfo2(void);
```

## 설명 (DESCRIPTION)

이 함수들은 `malloc(3)` 및 관련 함수들이 수행한 메모리 할당에 대한 정보를 담은 구조체의 복사본을 반환합니다. 각 함수가 반환하는 구조체는 같은 필드를 가지고 있습니다. 다만 예전 함수인 `mallinfo()`는 필드에 사용된 타입이 너무 작기 때문에 사용이 권장되지 않습니다(deprecated). (버그 항목 참고)

모든 할당이 이 함수들에 보이는 것은 아니라는 점에 유의하십시오. 버그 항목을 참고하고, 대신 `malloc_info(3)` 사용을 고려하십시오.

`mallinfo2()`가 반환하는 `mallinfo2` 구조체는 다음과 같이 정의되어 있습니다.

```c
struct mallinfo2 {
    size_t arena;     /* mmap이 아닌 방식으로 할당된 공간 (바이트) */
    size_t ordblks;   /* 빈 chunk 개수 */
    size_t smblks;    /* 빈 fastbin 블록 개수 */
    size_t hblks;     /* mmap된 영역 개수 */
    size_t hblkhd;    /* mmap된 영역에 할당된 공간 (바이트) */
    size_t usmblks;   /* 아래 설명 참고 */
    size_t fsmblks;   /* 해제된 fastbin 블록의 공간 (바이트) */
    size_t uordblks;  /* 할당된 공간 총합 (바이트) */
    size_t fordblks;  /* 빈 공간 총합 (바이트) */
    size_t keepcost;  /* 최상단의 반환 가능한 공간 (바이트) */
};
```

더 이상 권장되지 않는 `mallinfo()` 함수가 반환하는 `mallinfo` 구조체는 필드 타입이 `int`라는 점만 제외하면 완전히 같습니다.

구조체의 각 필드는 다음 정보를 담고 있습니다.

- **arena**: `mmap(2)` 이외의 방식으로 할당된 메모리의 총량(즉, 힙에 할당된 메모리)입니다. 이 수치에는 사용 중인 블록과 free list에 있는 블록이 모두 포함됩니다.
- **ordblks**: 일반(즉, fastbin이 아닌) 빈 블록의 개수입니다.
- **smblks**: fastbin 빈 블록의 개수입니다 (`mallopt(3)` 참고).
- **hblks**: 현재 `mmap(2)`을 사용해 할당된 블록의 개수입니다. (`mallopt(3)`의 `M_MMAP_THRESHOLD` 설명 참고)
- **hblkhd**: 현재 `mmap(2)`을 사용해 할당된 블록들의 바이트 수입니다.
- **usmblks**: 이 필드는 사용되지 않으며 항상 0입니다. 과거에는 할당된 공간의 "최고 수위(highwater mark)", 즉 지금까지 할당되었던 공간의 최대량(바이트)이었으며, 이 필드는 스레드를 사용하지 않는 환경에서만 유지되었습니다.
- **fsmblks**: fastbin 빈 블록들의 총 바이트 수입니다.
- **uordblks**: 사용 중인 할당들이 사용하는 총 바이트 수입니다.
- **fordblks**: 빈 블록들의 총 바이트 수입니다.
- **keepcost**: 힙 꼭대기에 있는 반환 가능한 빈 공간의 총량입니다. 이는 `malloc_trim(3)`으로 이상적인 경우(즉, 페이지 정렬 제약 등을 무시했을 때) 반환될 수 있는 최대 바이트 수입니다.

## 속성 (ATTRIBUTES)

이 절에서 사용된 용어에 대한 설명은 `attributes(7)`를 참고하십시오.

| 인터페이스 | 속성 | 값 |
|---|---|---|
| `mallinfo()`, `mallinfo2()` | 스레드 안전성 | MT-Unsafe init const:mallopt |

`mallinfo()`/`mallinfo2()`는 일부 전역 내부 객체에 접근합니다. 이 객체들이 원자적이지 않은 방식으로 수정되면 일관되지 않은 결과를 얻을 수 있습니다. `const:mallopt`에서 `mallopt`라는 식별자는, `mallopt()`가 전역 내부 객체를 원자적(atomic) 연산으로 수정하므로 `mallinfo()`/`mallinfo2()`를 충분히 안전하게 쓸 수 있다는 의미입니다. 다른 함수들은 원자적이지 않게 수정할 수도 있습니다.

## 표준 (STANDARDS)

없음.

## 역사 (HISTORY)

- `mallinfo()`: glibc 2.0. SVID.
- `mallinfo2()`: glibc 2.33.

## 버그 (BUGS)

정보는 메인 메모리 할당 영역(main arena)에 대해서만 반환됩니다. 다른 아레나에서의 할당은 제외됩니다. 다른 아레나에 대한 정보까지 포함하는 대안으로는 `malloc_stats(3)`와 `malloc_info(3)`를 참고하십시오.

예전 `mallinfo()` 함수가 반환하는 `mallinfo` 구조체의 필드들은 `int` 타입입니다. 하지만 일부 내부 관리 값들은 `long` 타입일 수 있기 때문에, 보고되는 값이 0을 넘어 순환(wrap around)하여 부정확해질 수 있습니다.

## 예제 (EXAMPLES)

아래 프로그램은 `mallinfo2()`를 사용해 메모리 블록을 할당하고 해제하기 전후의 메모리 할당 통계를 가져옵니다. 통계는 표준 출력에 표시됩니다.

처음 두 명령줄 인자는 `malloc(3)`으로 할당할 블록의 개수와 크기를 지정합니다.

나머지 세 인자는 할당된 블록 중 어떤 것을 `free(3)`로 해제할지 지정합니다. 이 세 인자는 선택 사항이며, 순서대로 다음을 지정합니다. 블록을 해제하는 루프에서 사용할 간격(기본값은 1로, 범위 내의 모든 블록을 해제한다는 의미), 해제할 첫 번째 블록의 순번(기본값 0, 즉 처음 할당된 블록), 해제할 마지막 블록의 순번보다 1 큰 수(기본값은 최대 블록 번호보다 1 큰 수). 이 세 인자를 생략하면 기본값에 의해 할당된 모든 블록이 해제됩니다.

다음 프로그램 실행 예에서는 100바이트 할당을 1000번 수행한 다음, 할당된 블록을 하나 걸러 하나씩 해제합니다.

```
$ ./a.out 1000 100 2;
============== Before allocating blocks ==============
Total non-mmapped bytes (arena):       0
# of free chunks (ordblks):            1
# of free fastbin blocks (smblks):     0
# of mapped regions (hblks):           0
Bytes in mapped regions (hblkhd):      0
Max. total allocated space (usmblks):  0
Free bytes held in fastbins (fsmblks): 0
Total allocated space (uordblks):      0
Total free space (fordblks):           0
Topmost releasable block (keepcost):   0

============== After allocating blocks ==============
Total non-mmapped bytes (arena):       135168
# of free chunks (ordblks):            1
# of free fastbin blocks (smblks):     0
# of mapped regions (hblks):           0
Bytes in mapped regions (hblkhd):      0
Max. total allocated space (usmblks):  0
Free bytes held in fastbins (fsmblks): 0
Total allocated space (uordblks):      104000
Total free space (fordblks):           31168
Topmost releasable block (keepcost):   31168

============== After freeing blocks ==============
Total non-mmapped bytes (arena):       135168
# of free chunks (ordblks):            501
# of free fastbin blocks (smblks):     0
# of mapped regions (hblks):           0
Bytes in mapped regions (hblkhd):      0
Max. total allocated space (usmblks):  0
Free bytes held in fastbins (fsmblks): 0
Total allocated space (uordblks):      52000
Total free space (fordblks):           83168
Topmost releasable block (keepcost):   31168
```

### 프로그램 소스

```c
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#define streq(...)  (strcmp(__VA_ARGS__) == 0)

static void
display_mallinfo2(void)
{
    struct mallinfo2 mi;

    mi = mallinfo2();

    printf("Total non-mmapped bytes (arena):       %zu\n", mi.arena);
    printf("# of free chunks (ordblks):            %zu\n", mi.ordblks);
    printf("# of free fastbin blocks (smblks):     %zu\n", mi.smblks);
    printf("# of mapped regions (hblks):           %zu\n", mi.hblks);
    printf("Bytes in mapped regions (hblkhd):      %zu\n", mi.hblkhd);
    printf("Max. total allocated space (usmblks):  %zu\n", mi.usmblks);
    printf("Free bytes held in fastbins (fsmblks): %zu\n", mi.fsmblks);
    printf("Total allocated space (uordblks):      %zu\n", mi.uordblks);
    printf("Total free space (fordblks):           %zu\n", mi.fordblks);
    printf("Topmost releasable block (keepcost):   %zu\n", mi.keepcost);
}

int
main(int argc, char *argv[])
{
#define MAX_ALLOCS 500000
    char *alloc[MAX_ALLOCS];
    size_t blockSize, numBlocks, freeBegin, freeEnd, freeStep;

    if (argc < 3 || streq(argv[1], "--help")) {
        fprintf(stderr, "%s num-blocks block-size [free-step "
                "[start-free [end-free]]]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    numBlocks = atoi(argv[1]);
    blockSize = atoi(argv[2]);
    freeStep = (argc > 3) ? atoi(argv[3]) : 1;
    freeBegin = (argc > 4) ? atoi(argv[4]) : 0;
    freeEnd = (argc > 5) ? atoi(argv[5]) : numBlocks;

    printf("============== Before allocating blocks ==============\n");
    display_mallinfo2();

    for (size_t j = 0; j < numBlocks; j++) {
        if (numBlocks >= MAX_ALLOCS) {
            fprintf(stderr, "Too many allocations\n");
            exit(EXIT_FAILURE);
        }

        alloc[j] = malloc(blockSize);
        if (alloc[j] == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
    }

    printf("\n============== After allocating blocks ==============\n");
    display_mallinfo2();

    for (size_t j = freeBegin; j < freeEnd; j += freeStep)
        free(alloc[j]);

    printf("\n============== After freeing blocks ==============\n");
    display_mallinfo2();

    exit(EXIT_SUCCESS);
}
```

## 관련 항목 (SEE ALSO)

`mmap(2)`, `malloc(3)`, `malloc_info(3)`, `malloc_stats(3)`, `malloc_trim(3)`, `mallopt(3)`

## 출처 (COLOPHON)

이 페이지는 man-pages(Linux 커널 및 C 라이브러리 사용자 공간 인터페이스 문서) 프로젝트의 일부입니다. 프로젝트에 대한 정보는 <https://www.kernel.org/doc/man-pages/> 에서 확인할 수 있습니다. 이 매뉴얼 페이지에 대한 버그 보고는 <https://git.kernel.org/pub/scm/docs/man-pages/man-pages.git/tree/CONTRIBUTING> 을 참고하십시오. 이 페이지는 <https://mirrors.edge.kernel.org/pub/linux/docs/man-pages/> 에서 2026-09-09에 가져온 man-pages-6.19.tar.gz 타르볼에서 얻은 것입니다.
