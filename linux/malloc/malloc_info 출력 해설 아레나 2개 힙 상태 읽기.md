# malloc\_info 출력 해설: 아레나 2개 힙 상태 읽기

Sep 29, 2026 · @보성

## 개요

이 프로세스의 힙은 아레나 2개, 총 1,220,496바이트(약 1.16MB)이고 그중 약 54%가 free 상태입니다. 다만 free 공간 대부분은 각 아레나 끝의 top 청크여서, 중간에 흩어진 조각은 약 150KB 수준으로 단편화는 심하지 않습니다.

`malloc_info()`는 glibc가 아레나별로 free 청크를 bin 종류와 크기별로 집계해 XML로 출력하는 함수입니다. `mallinfo2()`가 전체 합계만 주는 요약이라면, `malloc_info()`는 그 합계가 어느 아레나, 어느 bin에서 왔는지 보여 주는 상세 내역입니다.

이 문서의 모든 수치는 아래 한 번의 출력 스냅샷 기준입니다. 사용 중인 청크는 개별로 나오지 않고, tcache에 들어간 청크도 집계되지 않습니다.

## 전체 구조

출력은 아레나마다 `<heap>` 블록이 하나씩 있고, 맨 끝에 모든 아레나를 합친 전체 합계가 붙는 구조입니다.

| 블록 | 정체 | 사용 주체 | 시스템에서 받은 크기 |
| --- | --- | --- | --- |
| `<heap nr="0">` | 메인 아레나 (brk 기반 힙) | 메인 스레드 | 692,112 B |
| `<heap nr="1">` | 스레드 아레나 (mmap 서브힙 1개 위) | malloc을 쓰는 다른 스레드 | 528,384 B |
| 마지막 `<total>`, `<system>`, `<aspace>` | 두 아레나 합계 | - | 1,220,496 B |

메인 스레드는 처음부터 heap 0을 쓰고, 다른 스레드는 처음 malloc할 때 아레나를 배정받습니다. 아레나가 2개라는 것은 malloc을 쓰는 워커 스레드가 사실상 하나이거나, 스레드들이 종료되며 heap 1을 물려받고 있다는 뜻입니다. free는 어느 스레드가 하든 청크가 원래 속한 아레나로 돌아갑니다.

## 태그별 의미

| 태그 | 의미 | 단위 |
| --- | --- | --- |
| `<sizes>` | 아레나 안의 free 청크를 bin별로 나열한 목록 | - |
| `<size from to total count>` | bin 하나: 최소 크기, 최대 크기, 크기 합계, 청크 개수 | 바이트 / 개 |
| `<unsorted ...>` | unsorted bin: 방금 free되어 아직 크기별 bin으로 정리되지 않은 청크 | 바이트 / 개 |
| `<total type="fast">` | fastbin free 청크 합계 | 개 / 바이트 |
| `<total type="rest">` | fastbin 외 free 청크 합계 (top 청크 포함) | 개 / 바이트 |
| `<total type="mmap">` | 큰 요청을 직접 mmap으로 받은 청크 (전체 합계에만 표시) | 개 / 바이트 |
| `<system type="current">` | 현재 OS에서 받아 둔 아레나 메모리 | 바이트 |
| `<system type="max">` | 지금까지의 최대치 | 바이트 |
| `<aspace type="total">` | 아레나가 차지한 주소 공간 | 바이트 |
| `<aspace type="mprotect">` | 그중 읽기/쓰기 가능하게 연 부분 | 바이트 |
| `<aspace type="subheaps">` | 스레드 아레나가 쓰는 mmap 서브힙 개수 | 개 |

`system current`와 `max`가 같다는 것은 힙이 지금이 가장 큰 상태이고, 줄어든 적이 없다는 뜻입니다.

## size 줄 읽는 법

`<size>` 줄은 모양을 보면 어느 bin인지 구별됩니다. from과 to가 다르다는 것만으로는 fastbin이라고 판단할 수 없습니다.

| 모양 | bin 종류 | 이 출력의 예 | 읽는 법 |
| --- | --- | --- | --- |
| `from = 16k+1`, `to = 16(k+1)`, `total = count × to`, to ≤ 128 | fastbin | `17~32`, `33~48`, `113~128` | bin 크기로 만든 범위. 실제 청크 크기는 to 값 |
| from = to, 홀수 | small bin | `33`, `113`, `3713` | 1을 빼서 읽음 (33 → 32바이트) |
| from ≠ to, 홀수 | large bin | `8001~8113`, `3681~4033` | 한 bin에 여러 크기가 섞임. 1씩 빼서 읽음 |
| `<unsorted>` | unsorted bin | `209~4289` | 1씩 빼서 읽음 |

홀수가 나오는 이유는 `malloc_info`가 small/large/unsorted bin 청크의 크기 필드를 PREV\_INUSE 플래그 비트(1)가 붙은 채로 출력하기 때문입니다. 그래서 total도 청크 개수만큼 1씩 부풀어 있습니다. 예를 들어 unsorted의 total 31,164에서 개수 28을 빼면 실제 합계 31,136바이트이고, 16의 배수로 딱 떨어집니다.

## heap 0 (메인 아레나) 분석

메인 아레나의 free 353,012바이트 중 267,872바이트(76%)가 힙 끝의 top 청크 하나이고, 중간에 흩어진 조각은 20개, 약 85KB입니다. fastbin은 비어 있습니다.

| 항목 | 값 | 계산 |
| --- | --- | --- |
| 시스템에서 받은 크기 | 692,112 B | `system current` |
| free 합계 | 353,012 B (21개) | `total rest` |
| top 청크 | 267,872 B | 353,012 − 목록 합계 85,140 |
| 흩어진 free 조각 | 20개, 약 85,120 B | 목록 합계에서 플래그 비트 20 제외 |
| 사용 중 | 약 339,100 B | 692,112 − 353,012 |

흩어진 조각 중 큰 것은 34,736 B, 22,496 B, 8KB 대 2개이고, 나머지 16개는 32 B\~3.7KB의 작은 조각입니다. 크기가 제각각인 조각이 1개씩 있는 모양이라, 같은 크기를 반복해서 쓰기보다 다양한 크기를 가끔씩 할당·해제하는 패턴으로 보입니다.

top 청크 267,872 B는 mallinfo2의 `keepcost`와 정확히 일치합니다. 이 부분은 `malloc_trim(0)`을 호출하면 OS에 돌려줄 수 있습니다.

## heap 1 (스레드 아레나) 분석

스레드 아레나의 free 약 307KB 중 243,840바이트가 top 청크이고, 나머지는 fastbin 14개(800 B)와 흩어진 조각 34개(약 62.5KB)입니다. 그중 28개가 unsorted bin에 있어, 이 스레드가 할당·해제를 활발히 하고 있음을 보여 줍니다.

| 항목 | 값 | 계산 |
| --- | --- | --- |
| 시스템에서 받은 크기 | 528,384 B | `system current`, 서브힙 1개 |
| fastbin | 14개, 800 B | `total fast` |
| fastbin 외 free | 306,418 B (35개) | `total rest` |
| top 청크 | 243,840 B | 306,418 − 목록 합계 62,578 |
| 흩어진 free 조각 | 34개, 약 62,544 B | 목록 합계에서 플래그 비트 34 제외 |
| 사용 중 | 약 221,200 B | 528,384 − 800 − (306,418 − 34) |

fastbin 14개는 32 B 6개, 48 B 2개, 64 B 3개, 96 B 2개, 128 B 1개입니다. 같은 크기의 작은 청크를 한꺼번에 free해서 tcache(크기별 7칸)가 넘쳤고, 넘친 청크가 fastbin으로 내려온 것으로 보입니다.

unsorted bin의 28개(208 B\~4,288 B, 합계 31,136 B)는 최근 free된 뒤 아직 크기별 bin으로 정리되지 않은 청크입니다. 다음 malloc이 이 목록을 훑을 때 맞는 크기는 바로 재사용되고, 나머지는 small/large bin으로 옮겨집니다.

사용 중 수치에는 서브힙 앞머리의 `heap_info`와 아레나 관리 구조체도 포함되어 있어, 실제 프로그램 데이터는 이보다 조금 적습니다.

## mallinfo2 값과의 대조

비슷한 시점에 찍은 mallinfo2 값은 malloc\_info의 전체 합계와 대부분 그대로 대응합니다.

| mallinfo2 필드 | 값 | malloc\_info에서 온 곳 |
| --- | --- | --- |
| `arena` | 1,220,496 | 692,112 + 528,384 (두 아레나 `system current`) |
| `ordblks` | 56 | 전체 `total rest count` (21 + 35, top 청크 2개 포함) |
| `smblks` | 14 | 전체 `total fast count` |
| `fsmblks` | 800 | 전체 `total fast size` |
| `hblks` / `hblkhd` | 0 / 0 | `total mmap` |
| `keepcost` | 267,872 | heap 0의 top 청크 (메인 아레나만 집계) |
| `fordblks` | 664,288 | fast 800 + rest 659,430 = 660,230 (약 4KB 차이) |
| `uordblks` | 556,208 | arena − fordblks |
| `usmblks` | 0 | glibc에서 사용하지 않음 |

`fordblks`의 약 4KB 차이는 두 명령을 따로 실행한 사이에 프로그램이 계속 돌면서 생긴 것으로 보입니다. 정확히 맞추려면 한 gdb 세션 안에서 `mallinfo2`와 `malloc_info`를 연달아 호출하면 됩니다.

## 주의점과 결론

결론: 힙은 두 아레나를 합쳐 약 1.16MB로 작고, free 공간의 약 77%가 top 청크 2개에 모여 있어 단편화 문제는 없는 상태입니다.

해석할 때 주의할 점은 다음과 같습니다.

- **tcache는 어디에도 나오지 않습니다.** tcache에 들어간 free 청크는 "사용 중" 표시가 남아 있어, malloc\_info에 안 나오고 mallinfo2에서는 `uordblks`에 섞입니다. 실제 사용량은 표시보다 조금 적을 수 있습니다.
- **한 번의 스냅샷입니다.** fastbin과 unsorted bin은 몇 초 만에도 바뀝니다. 이후 측정에서 fastbin이 14개에서 1개로 줄어든 것도 확인되었습니다. 판단은 일정 간격으로 여러 번 찍은 `arena` 추이로 하는 것이 정확합니다.
- **판단 기준:** `arena`가 일정 선에서 멈추면 재사용이 정상, `arena`와 `uordblks`가 함께 계속 오르면 누수, `arena`만 오르고 `uordblks`는 그대로면 단편화를 의심합니다.

권장 사항은 두 가지입니다.

1. I/O가 주 병목이라 malloc 락 경쟁이 적으므로, `MALLOC_ARENA_MAX=2`(또는 `GLIBC_TUNABLES=glibc.malloc.arena_max=2`)로 스레드가 늘어도 아레나가 더 생기지 않게 막을 수 있습니다. 아레나마다 top 청크와 free 조각을 따로 들고 있으므로, 이것이 메모리를 줄이는 가장 직접적인 방법입니다.
2. 재사용하는 구조체는 원소마다 malloc하지 말고 배열이나 블록을 한 번 잡아 오브젝트 풀로 관리하면, 청크 헤더 낭비와 단편화를 함께 줄일 수 있습니다.

## 근거 자료

- [malloc\_info(3) Linux man page](https://man7.org/linux/man-pages/man3/malloc_info.3.html): 출력 형식
- [mallinfo(3) Linux man page](https://man7.org/linux/man-pages/man3/mallinfo.3.html): mallinfo2 각 필드 정의
- [glibc 위키 MallocInternals](https://sourceware.org/glibc/wiki/MallocInternals): 아레나, 서브힙, tcache, fastbin, unsorted/small/large bin 구조
- [glibc 소스 malloc/malloc.c](https://sourceware.org/git/?p=glibc.git;a=blob;f=malloc/malloc.c): `__malloc_info`의 bin별 집계와 fastbin 범위 표기, `int_mallinfo`
- [glibc 소스 malloc/arena.c](https://sourceware.org/git/?p=glibc.git;a=blob;f=malloc/arena.c): 스레드별 아레나 배정
- [mallopt(3) Linux man page](https://man7.org/linux/man-pages/man3/mallopt.3.html): `M_ARENA_MAX`, `M_MXFAST`, `M_MMAP_THRESHOLD`
- [malloc\_trim(3) Linux man page](https://man7.org/linux/man-pages/man3/malloc_trim.3.html): top 청크 반환
- [Heroku, Tuning glibc Memory Behavior](https://devcenter.heroku.com/articles/tuning-glibc-memory-behavior): `MALLOC_ARENA_MAX=2` 적용 사례
