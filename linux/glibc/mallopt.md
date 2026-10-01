# mallopt(3) — Linux 매뉴얼 페이지 (한국어 번역)

> 원문: [mallopt(3) — Linux manual page](https://man7.org/linux/man-pages/man3/mallopt.3.html) (Linux man-pages 6.19)
> 이 문서는 원문을 한국어로 번역한 것이며, 코드와 실행 결과 출력은 원문 그대로 두었습니다.

---

## 이름 (NAME)

mallopt - 메모리 할당 파라미터 설정

## 라이브러리 (LIBRARY)

표준 C 라이브러리 (`libc`, `-lc`)

## 개요 (SYNOPSIS)

```c
#include <malloc.h>

int mallopt(int param, int value);
```

## 설명 (DESCRIPTION)

`mallopt()` 함수는 메모리 할당 함수들(`malloc(3)` 참고)의 동작을 제어하는 파라미터를 조정합니다. `param` 인자는 수정할 파라미터를 지정하고, `value`는 그 파라미터의 새 값을 지정합니다.

`param`에는 다음 값들을 지정할 수 있습니다.

### M_ARENA_MAX

이 파라미터가 0이 아닌 값을 가지면, 생성될 수 있는 아레나(arena)의 최대 개수에 대한 강제 상한을 정의합니다. 아레나는 `malloc(3)`(및 유사한) 호출이 할당 요청을 처리하는 데 사용할 수 있는 메모리 풀을 말합니다. 아레나는 스레드 안전(thread safe)하므로 여러 메모리 요청을 동시에 받을 수 있습니다. 여기에는 스레드 수와 아레나 수 사이의 트레이드오프가 있습니다. 아레나가 많을수록 스레드당 경합은 줄어들지만 메모리 사용량은 늘어납니다.

이 파라미터의 기본값은 0이며, 이는 아레나 개수의 한도가 `M_ARENA_TEST` 설정에 따라 결정된다는 뜻입니다.

이 파라미터는 glibc 2.10부터 `--enable-experimental-malloc` 옵션을 통해, 그리고 glibc 2.15부터는 기본으로 사용할 수 있습니다. 일부 버전의 할당자에서는 생성되는 아레나 개수에 제한이 없었습니다(예: CentOS 5, RHEL 5).

최신 glibc 버전을 사용할 때, 애플리케이션에 따라 아레나 접근 시 높은 경합이 나타나는 경우가 있습니다. 이런 경우에는 `M_ARENA_MAX`를 스레드 수에 맞게 늘리는 것이 도움이 될 수 있습니다. 이는 tcmalloc이나 jemalloc이 취하는 전략(예: 스레드별 할당 풀)과 비슷한 동작입니다.

### M_ARENA_TEST

이 파라미터는 아레나가 몇 개 생성되었을 때 시스템 구성을 검사하여 생성 가능한 아레나 개수의 강제 상한을 결정할지를, 생성된 아레나 개수 단위로 지정합니다. (아레나의 정의는 `M_ARENA_MAX`를 참고하십시오.)

아레나 강제 상한의 계산 방식은 구현에 따라 다르며, 보통 사용 가능한 CPU 수의 배수로 계산됩니다. 상한이 한 번 계산되면 그 결과는 최종값이 되며 전체 아레나 개수를 제한합니다.

`M_ARENA_TEST` 파라미터의 기본값은 `sizeof(long)`이 4인 시스템에서는 2이고, 그 외에는 8입니다.

이 파라미터는 glibc 2.10부터 `--enable-experimental-malloc` 옵션을 통해, 그리고 glibc 2.15부터는 기본으로 사용할 수 있습니다.

`M_ARENA_MAX`가 0이 아닌 값을 가지면 `M_ARENA_TEST` 값은 사용되지 않습니다.

### M_CHECK_ACTION

이 파라미터를 설정하면 여러 종류의 프로그래밍 오류(예: 같은 포인터를 두 번 해제하는 경우)가 감지되었을 때 glibc가 어떻게 대응할지를 제어합니다. 이 파라미터에 지정한 값의 하위 3비트(비트 2, 1, 0)가 glibc의 동작을 다음과 같이 결정합니다.

- **비트 0**: 이 비트가 설정되어 있으면 오류에 대한 세부 정보를 담은 한 줄 메시지를 stderr에 출력합니다. 메시지는 `"*** glibc detected ***"` 문자열로 시작하며, 이어서 프로그램 이름, 오류가 감지된 메모리 할당 함수의 이름, 오류에 대한 간단한 설명, 오류가 감지된 메모리 주소가 뒤따릅니다.
- **비트 1**: 이 비트가 설정되어 있으면 비트 0에 의해 지정된 오류 메시지를 출력한 뒤 `abort(3)`를 호출하여 프로그램을 종료합니다. glibc 2.4부터는 비트 0도 함께 설정되어 있으면, 오류 메시지 출력과 abort 사이에 `backtrace(3)` 방식의 스택 트레이스와 `/proc/pid/maps` 형식(`proc(5)` 참고)의 프로세스 메모리 매핑도 출력합니다.
- **비트 2** (glibc 2.4부터): 이 비트는 비트 0도 설정된 경우에만 효과가 있습니다. 이 비트가 설정되어 있으면 오류를 설명하는 한 줄 메시지가 오류가 감지된 함수 이름과 오류에 대한 간단한 설명만 담도록 단순화됩니다.

`value`의 나머지 비트는 무시됩니다.

위 내용을 종합하면, `M_CHECK_ACTION`에 의미 있는 숫자 값은 다음과 같습니다.

| 값 | 동작 |
|---|---|
| 0 | 오류 상황을 무시하고 실행을 계속합니다 (결과는 정의되지 않음). |
| 1 | 자세한 오류 메시지를 출력하고 실행을 계속합니다. |
| 2 | 프로그램을 중단(abort)합니다. |
| 3 | 자세한 오류 메시지, 스택 트레이스, 메모리 매핑을 출력하고 프로그램을 중단합니다. |
| 5 | 간단한 오류 메시지를 출력하고 실행을 계속합니다. |
| 7 | 간단한 오류 메시지, 스택 트레이스, 메모리 매핑을 출력하고 프로그램을 중단합니다. |

glibc 2.3.4부터 `M_CHECK_ACTION` 파라미터의 기본값은 3입니다. glibc 2.3.3 이하에서는 기본값이 1입니다.

`M_CHECK_ACTION`을 0이 아닌 값으로 쓰는 것이 유용할 수 있습니다. 그렇지 않으면 크래시가 훨씬 나중에 발생할 수 있고, 그 경우 문제의 진짜 원인을 추적하기가 매우 어려워지기 때문입니다.

### M_MMAP_MAX

이 파라미터는 `mmap(2)`을 사용해 동시에 처리될 수 있는 할당 요청의 최대 개수를 지정합니다. 이 파라미터가 존재하는 이유는, 일부 시스템에서는 `mmap(2)`이 사용하는 내부 테이블의 수가 제한되어 있어 이를 몇 개 이상 사용하면 성능이 저하될 수 있기 때문입니다.

기본값은 65,536이며, 이 값에 특별한 의미는 없고 단지 안전장치 역할만 합니다. 이 파라미터를 0으로 설정하면 큰 할당 요청을 처리할 때 `mmap(2)`을 사용하지 않게 됩니다.

### M_MMAP_THRESHOLD

`M_MMAP_THRESHOLD`로 지정한 한도(바이트 단위) 이상이면서 free list로 충족할 수 없는 할당에 대해서는, 메모리 할당 함수들이 `sbrk(2)`로 프로그램 브레이크를 늘리는 대신 `mmap(2)`을 사용합니다.

`mmap(2)`을 사용한 메모리 할당에는, 할당된 메모리 블록을 언제나 독립적으로 시스템에 반환할 수 있다는 큰 장점이 있습니다. (반면 힙은 꼭대기 쪽 메모리가 해제된 경우에만 줄일 수 있습니다.) 한편 `mmap(2)` 사용에는 몇 가지 단점도 있습니다. 해제된 공간이 이후 할당에서 재사용될 수 있도록 free list에 들어가지 않고, `mmap(2)` 할당은 페이지 단위로 정렬되어야 하므로 메모리가 낭비될 수 있으며, 커널이 `mmap(2)`으로 할당된 메모리를 0으로 채우는 비싼 작업을 수행해야 합니다. 이러한 요소들의 균형을 맞춘 결과, `M_MMAP_THRESHOLD` 파라미터의 기본값은 128*1024로 정해졌습니다.

이 파라미터의 하한은 0입니다. 상한은 `DEFAULT_MMAP_THRESHOLD_MAX`로, 32비트 시스템에서는 512*1024, 64비트 시스템에서는 4\*1024\*1024\*sizeof(long)입니다.

**참고**: 현재 glibc는 기본적으로 동적 mmap 임계값을 사용합니다. 임계값의 초기값은 128*1024이지만, 현재 임계값보다 크고 `DEFAULT_MMAP_THRESHOLD_MAX` 이하인 블록이 해제되면 임계값이 그 해제된 블록의 크기로 상향 조정됩니다. 동적 mmap 임계값이 동작 중일 때는 힙 트리밍 임계값도 동적 mmap 임계값의 두 배로 동적으로 조정됩니다. `M_TRIM_THRESHOLD`, `M_TOP_PAD`, `M_MMAP_THRESHOLD`, `M_MMAP_MAX` 파라미터 중 하나라도 설정되면 mmap 임계값의 동적 조정은 비활성화됩니다.

### M_MXFAST (glibc 2.3부터)

"fastbin"을 사용해 처리되는 메모리 할당 요청의 상한을 설정합니다. (이 파라미터의 측정 단위는 바이트입니다.) fastbin은 같은 크기의 해제된 메모리 블록을 인접한 빈 블록과 병합하지 않고 보관하는 저장 영역입니다. 이후 같은 크기의 블록을 다시 할당할 때 fastbin에서 매우 빠르게 처리할 수 있지만, 메모리 단편화와 프로그램의 전체 메모리 사용량이 증가할 수 있습니다.

이 파라미터의 기본값은 64\*sizeof(size_t)/4 (즉 32비트 아키텍처에서는 64)입니다. 이 파라미터의 범위는 0부터 80\*sizeof(size_t)/4까지입니다. `M_MXFAST`를 0으로 설정하면 fastbin 사용이 비활성화됩니다.

### M_PERTURB (glibc 2.4부터)

이 파라미터가 0이 아닌 값으로 설정되면, 할당된 메모리의 바이트들(`calloc(3)`을 통한 할당은 제외)이 `value`의 최하위 바이트 값의 보수(complement)로 초기화되고, 할당된 메모리가 `free(3)`로 해제될 때는 해제된 바이트들이 `value`의 최하위 바이트 값으로 설정됩니다. 이는 프로그램이 할당된 메모리가 0으로 초기화되어 있다고 잘못 가정하거나, 이미 해제된 메모리의 값을 재사용하는 오류를 찾아내는 데 유용할 수 있습니다.

이 파라미터의 기본값은 0입니다.

### M_TOP_PAD

이 파라미터는 프로그램 브레이크를 수정하기 위해 `sbrk(2)`를 호출할 때 사용할 패딩의 양을 정의합니다. (이 파라미터의 측정 단위는 바이트입니다.) 이 파라미터는 다음 상황에서 효과가 있습니다.

- 프로그램 브레이크가 늘어날 때, `sbrk(2)` 요청에 `M_TOP_PAD` 바이트가 추가됩니다.
- `free(3)` 호출의 결과로 힙이 트리밍될 때(`M_TRIM_THRESHOLD` 설명 참고), 힙 꼭대기에 이만큼의 빈 공간이 보존됩니다.

어느 경우든 패딩의 양은 항상 시스템 페이지 경계에 맞게 반올림됩니다.

`M_TOP_PAD`를 조정하는 것은 시스템 호출 횟수 증가(값을 낮게 설정한 경우)와 힙 꼭대기의 미사용 메모리 낭비(값을 높게 설정한 경우) 사이의 트레이드오프입니다.

이 파라미터의 기본값은 128*1024입니다.

### M_TRIM_THRESHOLD

힙 꼭대기의 연속된 빈 메모리 양이 충분히 커지면, `free(3)`는 `sbrk(2)`를 사용해 이 메모리를 시스템에 반환합니다. (이는 상당한 양의 메모리를 해제한 뒤에도 오랫동안 실행을 계속하는 프로그램에서 유용할 수 있습니다.) `M_TRIM_THRESHOLD` 파라미터는 `sbrk(2)`로 힙을 트리밍하기 전에 이 메모리 블록이 도달해야 하는 최소 크기(바이트 단위)를 지정합니다.

이 파라미터의 기본값은 128*1024입니다. `M_TRIM_THRESHOLD`를 -1로 설정하면 트리밍이 완전히 비활성화됩니다.

`M_TRIM_THRESHOLD`를 조정하는 것은 시스템 호출 횟수 증가(값을 낮게 설정한 경우)와 힙 꼭대기의 미사용 메모리 낭비(값을 높게 설정한 경우) 사이의 트레이드오프입니다.

### 환경 변수 (Environment variables)

`mallopt()`가 제어하는 파라미터 중 일부는 환경 변수를 정의하여 수정할 수도 있습니다. 이 환경 변수들을 사용하면 프로그램의 소스 코드를 변경할 필요가 없다는 장점이 있습니다. 효과가 있으려면 이 변수들은 메모리 할당 함수가 처음 호출되기 전에 정의되어 있어야 합니다. (같은 파라미터를 `mallopt()`로도 조정한 경우에는 `mallopt()` 설정이 우선합니다.) 보안상의 이유로, 이 변수들은 set-user-ID 및 set-group-ID 프로그램에서는 무시됩니다.

환경 변수는 다음과 같습니다 (일부 변수 이름 끝에 밑줄이 붙어 있다는 점에 유의하십시오).

- **MALLOC_ARENA_MAX**: `mallopt()`의 `M_ARENA_MAX`와 같은 파라미터를 제어합니다.
- **MALLOC_ARENA_TEST**: `mallopt()`의 `M_ARENA_TEST`와 같은 파라미터를 제어합니다.
- **MALLOC_CHECK_**: 이 환경 변수는 `mallopt()`의 `M_CHECK_ACTION`과 같은 파라미터를 제어합니다. 이 변수가 0이 아닌 값으로 설정되면, 특수한 메모리 할당 함수 구현이 사용됩니다. (이는 `malloc_hook(3)` 기능을 이용해 이루어집니다.) 이 구현은 추가적인 오류 검사를 수행하지만 표준 메모리 할당 함수들보다 느립니다. (이 구현이 가능한 모든 오류를 감지하지는 못하며, 메모리 누수는 여전히 발생할 수 있습니다.)

  이 환경 변수에 지정하는 값은 한 자리 숫자여야 하며, 그 의미는 `M_CHECK_ACTION`에서 설명한 것과 같습니다. 첫 번째 숫자 뒤의 문자는 모두 무시됩니다.

  보안상의 이유로, `MALLOC_CHECK_`의 효과는 set-user-ID 및 set-group-ID 프로그램에서 기본적으로 비활성화됩니다. 하지만 `/etc/suid-debug` 파일이 존재하면(파일 내용은 상관없음) set-user-ID 및 set-group-ID 프로그램에서도 `MALLOC_CHECK_`가 효과를 가집니다.
- **MALLOC_MMAP_MAX_**: `mallopt()`의 `M_MMAP_MAX`와 같은 파라미터를 제어합니다.
- **MALLOC_MMAP_THRESHOLD_**: `mallopt()`의 `M_MMAP_THRESHOLD`와 같은 파라미터를 제어합니다.
- **MALLOC_PERTURB_**: `mallopt()`의 `M_PERTURB`와 같은 파라미터를 제어합니다.
- **MALLOC_TRIM_THRESHOLD_**: `mallopt()`의 `M_TRIM_THRESHOLD`와 같은 파라미터를 제어합니다.
- **MALLOC_TOP_PAD_**: `mallopt()`의 `M_TOP_PAD`와 같은 파라미터를 제어합니다.

## 반환값 (RETURN VALUE)

성공하면 `mallopt()`는 1을 반환합니다. 오류 시에는 0을 반환합니다.

## 오류 (ERRORS)

오류 시 `errno`는 설정되지 않습니다.

## 버전 (VERSIONS)

비슷한 함수가 여러 System V 계열 시스템에 존재하지만, `param`에 쓸 수 있는 값의 범위는 시스템마다 다릅니다. SVID는 `M_MXFAST`, `M_NLBLKS`, `M_GRAIN`, `M_KEEP` 옵션을 정의했지만, glibc에는 이 중 첫 번째만 구현되어 있습니다.

## 표준 (STANDARDS)

없음.

## 역사 (HISTORY)

glibc 2.0.

## 버그 (BUGS)

`param`에 잘못된 값을 지정해도 오류가 발생하지 않습니다.

glibc 구현 내부의 계산 오류로 인해, 다음과 같은 형태의 호출은

```c
mallopt(M_MXFAST, n)
```

크기가 `n` 이하인 모든 할당에 대해 fastbin이 사용되도록 만들지 못합니다. 원하는 결과를 보장하려면 `n`을 (2k+1)\*sizeof(size_t)(k는 정수) 이상인 다음 배수로 올림해야 합니다.

`mallopt()`로 `M_PERTURB`를 설정하면, 예상대로 할당된 메모리의 바이트들은 `value`의 바이트 값의 보수로 초기화되고, 그 메모리가 해제될 때 해당 영역의 바이트들은 `value`에 지정한 바이트 값으로 초기화됩니다. 하지만 구현에 sizeof(size_t)만큼 어긋나는(off-by-sizeof(size_t)) 오류가 있어서, `free(p)` 호출로 해제되는 메모리 블록을 정확히 초기화하는 대신 `p+sizeof(size_t)`에서 시작하는 블록이 초기화됩니다.

## 예제 (EXAMPLES)

아래 프로그램은 `M_CHECK_ACTION`의 사용법을 보여줍니다. 프로그램에 (정수) 명령줄 인자가 주어지면, 그 인자를 사용해 `M_CHECK_ACTION` 파라미터를 설정합니다. 그런 다음 프로그램은 메모리 블록을 하나 할당하고, 그것을 두 번 해제합니다(오류).

다음 셸 세션은 `M_CHECK_ACTION`의 기본값으로 glibc에서 이 프로그램을 실행했을 때 어떤 일이 일어나는지 보여줍니다.

```
$ ./a.out;
main(): returned from first free() call
*** glibc detected *** ./a.out: double free or corruption (top): 0x09d30008 ***
======= Backtrace: =========
/lib/libc.so.6(+0x6c501)[0x523501]
/lib/libc.so.6(+0x6dd70)[0x524d70]
/lib/libc.so.6(cfree+0x6d)[0x527e5d]
./a.out[0x80485db]
/lib/libc.so.6(__libc_start_main+0xe7)[0x4cdce7]
./a.out[0x8048471]
======= Memory map: ========
001e4000-001fe000 r-xp 00000000 08:06 1083555    /lib/libgcc_s.so.1
001fe000-001ff000 r--p 00019000 08:06 1083555    /lib/libgcc_s.so.1
[some lines omitted]
b7814000-b7817000 rw-p 00000000 00:00 0
bff53000-bff74000 rw-p 00000000 00:00 0          [stack]
Aborted (core dumped)
```

다음 실행 결과들은 `M_CHECK_ACTION`에 다른 값을 사용했을 때의 결과를 보여줍니다.

```
$ ./a.out 1;             # 오류를 진단하고 계속 실행
main(): returned from first free() call
*** glibc detected *** ./a.out: double free or corruption (top): 0x09cbe008 ***
main(): returned from second free() call
$ ./a.out 2;             # 오류 메시지 없이 중단
main(): returned from first free() call
Aborted (core dumped)
$ ./a.out 0;             # 오류를 무시하고 계속 실행
main(): returned from first free() call
main(): returned from second free() call
```

다음 실행은 `MALLOC_CHECK_` 환경 변수를 사용해 같은 파라미터를 설정하는 방법을 보여줍니다.

```
$ MALLOC_CHECK_=1 ./a.out;
main(): returned from first free() call
*** glibc detected *** ./a.out: free(): invalid pointer: 0x092c2008 ***
main(): returned from second free() call
```

### 프로그램 소스

```c
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
    char *p;

    if (argc > 1) {
        if (mallopt(M_CHECK_ACTION, atoi(argv[1])) != 1) {
            fprintf(stderr, "mallopt() failed");
            exit(EXIT_FAILURE);
        }
    }

    p = malloc(1000);
    if (p == NULL) {
        fprintf(stderr, "malloc() failed");
        exit(EXIT_FAILURE);
    }

    free(p);
    printf("%s(): returned from first free() call\n", __func__);

    free(p);
    printf("%s(): returned from second free() call\n", __func__);

    exit(EXIT_SUCCESS);
}
```

## 관련 항목 (SEE ALSO)

`mmap(2)`, `sbrk(2)`, `mallinfo(3)`, `malloc(3)`, `malloc_hook(3)`, `malloc_info(3)`, `malloc_stats(3)`, `malloc_trim(3)`, `mcheck(3)`, `mtrace(3)`, `posix_memalign(3)`

## 출처 (COLOPHON)

이 페이지는 man-pages(Linux 커널 및 C 라이브러리 사용자 공간 인터페이스 문서) 프로젝트의 일부입니다. 프로젝트에 대한 정보는 <https://www.kernel.org/doc/man-pages/> 에서 확인할 수 있습니다. 이 매뉴얼 페이지에 대한 버그 보고는 <https://git.kernel.org/pub/scm/docs/man-pages/man-pages.git/tree/CONTRIBUTING> 을 참고하십시오. 이 페이지는 <https://mirrors.edge.kernel.org/pub/linux/docs/man-pages/> 에서 2026-09-09에 가져온 man-pages-6.19.tar.gz 타르볼에서 얻은 것입니다.
