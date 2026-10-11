# glibc malloc 질문·답변 정리

mallopt 사용법부터 힙 내부 구조(top chunk, fastbin, small/large bin)까지 대화에서 질문한 순서대로 정리한 노트입니다.

함께 만든 번역 문서: `mallopt.md`, `mallinfo.md`, `bins-and-memory-allocations.md`

## 목차

1. mallopt로 옵션 주는 법
2. 첫 malloc 전에 설정해야 하나요?
3. config 파일을 읽어서 설정하려면?
4. 환경 변수(GLIBC_TUNABLES)로 설정하는 예시
5. mmap threshold는 왜 동적으로 바뀌나요?
6. set-user-ID / set-group-ID 프로그램이 뭔가요?
7. fastbin이 뭔가요?
8. fordblks가 fastbin을 말하는 건가요?
9. mallinfo2는 몇 버전에 나왔나요?
10. 힙 꼭대기(top chunk)와 fastbin의 전체 그림
11. fastbin과 일반 bin을 분리하는 이유
12. fastbin에 크기 제한이 있나요?
13. 일반 bin 구분은 glibc 몇 버전 이야기인가요?
14. small bin / large bin이 뭔가요?
15. 참고 링크 모음

---

## 1. mallopt로 옵션 주는 법

`#include <malloc.h>` 후 `mallopt(int param, int value)`로 호출합니다. param에 바꿀 파라미터, value에 새 값을 줍니다. glibc에서는 성공 시 1, 실패 시 0을 반환합니다(errno는 설정 안 됨).

```c
#include <malloc.h>
#include <stdio.h>

int main(void) {
    if (mallopt(M_MMAP_THRESHOLD, 256 * 1024) == 0)
        fprintf(stderr, "M_MMAP_THRESHOLD 설정 실패\n");
    mallopt(M_TRIM_THRESHOLD, 512 * 1024);
    mallopt(M_ARENA_MAX, 2);
    /* ... */
}
```

자주 쓰는 옵션:

| 옵션 | 의미 |
|---|---|
| `M_ARENA_MAX` | 아레나 최대 개수. 멀티스레드 메모리 사용량 절감에 자주 사용 |
| `M_MMAP_THRESHOLD` | 이 크기 이상 요청은 mmap으로 할당 |
| `M_TRIM_THRESHOLD` | 힙 꼭대기 빈 공간이 이 값을 넘으면 OS에 반환 (-1이면 끔) |
| `M_TOP_PAD` | sbrk로 힙을 늘릴 때 추가로 확보할 여유 공간 |
| `M_MMAP_MAX` | mmap 할당 최대 개수 (0이면 mmap 사용 안 함) |
| `M_MXFAST` | fastbin에 들어갈 최대 크기 (0이면 fastbin 끔) |
| `M_PERTURB` | 할당/해제 메모리를 특정 바이트로 채움 (디버깅용) |

Android(bionic)는 옵션 체계가 다르며, API 레벨 26부터 mallopt 사용 가능하고 scudo 전용 옵션이 따로 있습니다.

## 2. 첫 malloc 전에 설정해야 하나요?

"권장"이지 "필수"는 아닙니다. 방법에 따라 다릅니다.

- **환경 변수 방식은 필수**: 첫 메모리 할당 함수 호출 전에 정의되어 있어야 효과가 있습니다.
- **mallopt() 호출은 나중에 해도 대부분 동작**: 호출 이후의 할당부터 적용됩니다. 이미 할당된 메모리에는 소급 적용되지 않습니다.
  - 예: `M_ARENA_MAX`를 줄여도 이미 만들어진 아레나는 사라지지 않고, 새로 생기는 것만 제한됩니다.
- **실무 기준**: 스레드를 만들기 전, 큰 할당을 시작하기 전에만 설정하면 충분합니다.

`main()` 진입 전에도 libc 초기화, C++ 전역 생성자, stdio 버퍼 등에서 malloc이 호출될 수 있어서 "첫 malloc 전"을 엄밀히 보장하긴 어렵습니다. 더 일찍 적용하려면 constructor를 씁니다.

```c
__attribute__((constructor))
static void init_malloc_opts(void) {
    mallopt(M_ARENA_MAX, 2);
}
```

## 3. config 파일을 읽어서 설정하려면?

`fopen()` 자체가 내부적으로 malloc을 쓰기 때문에 애매하게 느껴질 수 있지만, 대부분 문제되지 않습니다. config 읽는 동안의 작은 할당 몇 개만 기본 설정으로 처리되기 때문입니다. **"config 읽기 → mallopt 적용 → 스레드 생성·본 작업"** 순서면 충분합니다.

더 깔끔하게 하고 싶다면 세 가지 방법이 있습니다.

1. **open()/read() 시스템 콜 + 스택 버퍼**: malloc을 쓰지 않으므로 constructor에서 안전하게 사용 가능. `strstr`, `strtol`도 malloc을 안 씁니다.
2. **re-exec 방식**: config를 읽고 `GLIBC_TUNABLES`를 설정한 뒤 `execv()`로 자기 자신을 다시 실행. 가장 확실하지만 구조가 번거롭고, 무한 재실행 방지용 표시가 필요합니다.
3. **실행 스크립트/systemd에서 환경 변수로 전달**: 운영 환경에서 가장 흔한 방식. (다음 항목 참고)

## 4. 환경 변수(GLIBC_TUNABLES)로 설정하는 예시

**config 파일** (`/etc/myapp/malloc.conf`)

```
arena_max=2
trim_threshold=524288
mmap_threshold=262144
```

**래퍼 스크립트**

```bash
#!/bin/sh
CONF=/etc/myapp/malloc.conf
T=""
while IFS='=' read -r k v; do
    case "$k" in ''|\#*) continue ;; esac
    T="${T:+$T:}glibc.malloc.$k=$v"
done < "$CONF"
export GLIBC_TUNABLES="$T"
exec /usr/local/bin/myapp "$@"
```

`exec`를 쓰면 셸이 실제 프로그램으로 교체되어 PID가 유지되고 시그널도 직접 전달됩니다.

**systemd로 띄우는 경우**

```ini
# /etc/myapp/malloc.env
GLIBC_TUNABLES=glibc.malloc.arena_max=2:glibc.malloc.trim_threshold=524288
```

```ini
# /etc/systemd/system/myapp.service
[Service]
EnvironmentFile=/etc/myapp/malloc.env
ExecStart=/usr/local/bin/myapp
```

**적용 확인**

- `tr '\0' '\n' < /proc/<PID>/environ | grep GLIBC`
- glibc 2.33+: `GLIBC_TUNABLES=... /lib64/ld-linux-x86-64.so.2 --list-tunables`

**주의점**

- `GLIBC_TUNABLES`는 glibc 2.26부터 지원. 그 이전은 `MALLOC_ARENA_MAX=2` 같은 기존 환경 변수 사용.
- 같은 파라미터를 mallopt()로도 설정하면 mallopt()가 우선합니다.
- set-user-ID/set-group-ID 프로그램에서는 이 환경 변수들이 무시됩니다.

## 5. mmap threshold는 왜 동적으로 바뀌나요?

mmap과 힙(sbrk)에는 각각 장단점이 있어서, 고정값 하나로는 모든 프로그램의 할당 패턴에 맞출 수 없기 때문입니다.

| | mmap | 힙(sbrk) |
|---|---|---|
| OS 반환 | 블록마다 언제든 개별 반환 가능 | 꼭대기가 비었을 때만 줄일 수 있음 |
| 재사용 | free list에 안 들어가 재사용 불가 | free list로 재사용 |
| 비용 | 페이지 정렬 낭비, 커널이 0으로 채우는 비용 | 상대적으로 저렴 |

**동적 조정 방식**: 초기값은 128KB. 현재 임계값보다 크고 `DEFAULT_MMAP_THRESHOLD_MAX`(64비트 32MB, 32비트 512KB) 이하인 블록이 free되면 임계값이 그 크기로 올라갑니다. trim 임계값도 mmap 임계값의 2배로 함께 조정됩니다.

**발상**: "이 크기를 해제했다는 건 비슷한 크기를 또 할당할 가능성이 높다" → 다음부터 힙에서 처리해 재사용. 큰 임시 버퍼를 반복 할당하는 프로그램에서 mmap/munmap 반복을 막아줍니다.

**한계**: 임계값은 올라가기만 하고 내려가지 않습니다. 장시간 서버에서 RSS가 높게 유지되거나 단편화가 생길 수 있어, 이럴 때 값을 고정합니다.

**중요**: `M_TRIM_THRESHOLD`, `M_TOP_PAD`, `M_MMAP_THRESHOLD`, `M_MMAP_MAX` 중 **하나라도 설정하면 동적 조정이 꺼집니다.** 동적 방식을 유지하면서 아레나만 줄이려면 `arena_max`만 설정하세요.

## 6. set-user-ID / set-group-ID 프로그램이 뭔가요?

실행한 사람이 아니라 **파일 소유자(또는 그룹)의 권한으로 실행되는 프로그램**입니다.

```bash
$ ls -l /usr/bin/passwd
-rwsr-xr-x 1 root root ... /usr/bin/passwd
#   ^ x 대신 s → setuid 비트
```

- 대표 예: `passwd`는 일반 사용자가 실행해도 root 권한으로 `/etc/shadow`를 수정합니다.
- 설정: `chmod u+s 파일`(setuid), `chmod g+s 파일`(setgid)
- 원리: 실제(real) UID는 실행한 사람, 유효(effective) UID는 파일 소유자가 됩니다. saved set-user-ID로 권한을 필요할 때만 켜고 끌 수 있습니다.

**malloc 환경 변수가 무시되는 이유**: 환경 변수는 실행하는 사람이 마음대로 정할 수 있으므로, 일반 사용자가 root 권한 프로그램의 동작을 조작하는 데 악용될 수 있기 때문입니다. `LD_PRELOAD` 등도 같은 이유로 제한됩니다. 일반 서버 프로그램은 setuid가 아니므로 신경 쓰지 않아도 됩니다.

## 7. fastbin이 뭔가요?

glibc malloc이 **작은 크기의 해제된 chunk를 빠르게 재사용하기 위해 따로 모아두는 목록**입니다.

- free된 메모리는 바로 OS에 가지 않고 크기별 목록(bin)에 보관됩니다. bin 종류: fastbin, unsorted, small, large (+ glibc 2.26부터 tcache)
- fastbin의 특징
  - 단일 연결 리스트 (중간에서 뺄 일이 없으므로)
  - LIFO (순서가 중요하지 않으므로 더 빠른 방식)
  - inuse 비트를 유지 → **이웃 빈 chunk와 병합되지 않음**
- 장점: free/malloc이 리스트 맨 앞에 넣고 빼기만 하는 O(1) 연산
- 단점: 병합하지 않으니 단편화 발생 → `malloc_consolidate()`가 특정 시점에 한꺼번에 병합
- tcache와의 관계: malloc은 tcache → fastbin 순서로 확인. fastbin에서 꺼낼 때 같은 크기가 남아 있으면 tcache로 옮겨 담음

## 8. fordblks가 fastbin을 말하는 건가요?

아닙니다. `fordblks`는 **모든 빈 블록의 총 바이트 수**입니다.

| 필드 | 의미 |
|---|---|
| `fordblks` | 빈 블록 전체 총 바이트 (fastbin + 일반 bin + top chunk) |
| `ordblks` | 일반(fastbin 아닌) 빈 블록 개수 |
| `smblks` | fastbin 빈 블록 개수 |
| `fsmblks` | fastbin 빈 블록 총 바이트 |
| `uordblks` | 사용 중인 할당의 총 바이트 |
| `keepcost` | 힙 꼭대기에서 malloc_trim으로 반환 가능한 최대 바이트 |
| `arena` | mmap 아닌 힙 전체 크기 |
| `hblks` / `hblkhd` | mmap 할당 개수 / 바이트 |

주의점:

- tcache에 있는 chunk는 내부적으로 "사용 중"으로 표시되어 `uordblks`에 섞여 집계됩니다.
- **mallinfo/mallinfo2는 메인 아레나 정보만 반환합니다.** 다른 아레나(다른 스레드)는 빠지므로, 전체를 보려면 `malloc_stats(3)`나 `malloc_info(3)`를 쓰세요. (대화 중 "전체 아레나 합산"이라고 잘못 설명했다가 정정한 부분입니다.)
- 옛 `mallinfo()`는 필드가 `int`라 2GB를 넘으면 값이 틀어집니다.
- 진단 팁: `fordblks`는 큰데 `keepcost`가 작다면 → 빈 메모리가 힙 중간에 흩어져 있어 OS에 못 돌려주는 상태

## 9. mallinfo2는 몇 버전에 나왔나요?

**glibc 2.33** (2021년 2월). `mallinfo()`는 glibc 2.0부터 있었습니다.

- 있음: Ubuntu 22.04(2.35), RHEL 9(2.34)
- 없음: Ubuntu 20.04(2.31), RHEL 8(2.28)
- 버전 확인: `ldd --version`

```c
#if __GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 33)
    struct mallinfo2 mi = mallinfo2();
#else
    struct mallinfo mi = mallinfo();
#endif
```

## 10. 힙 꼭대기(top chunk)와 fastbin의 전체 그림

```
낮은 주소                                                  높은 주소
┌────────┬──────┬────────┬────────┬────────┬───────────────────┐
│ 사용 중 │ free │ 사용 중 │  free  │ 사용 중 │     top chunk     │ ← program break
└────────┴──┬───┴────────┴───┬────┴────────┴───────────────────┘
            │                │                  sbrk로 늘림 / trim으로 반환
            ▼                ▼
    [tcache][fastbin]    [일반 bin: unsorted/small/large]

별도 영역: [mmap 영역] ← M_MMAP_THRESHOLD 이상 큰 할당, free 시 바로 OS 반환
```

- **힙**: 낮은 주소에서 높은 주소로 자라는 연속 메모리. 끝 경계가 program break이고 `sbrk()`로 늘립니다.
- **top chunk(힙 꼭대기)**: 아직 아무에게도 안 나눠준 맨 끝의 빈 공간. **OS에 돌려줄 수 있는 건 여기뿐**입니다. `M_TRIM_THRESHOLD`보다 커지면 반환됩니다.
- **중간의 free 조각**: 앞뒤가 사용 중 chunk에 막혀 OS에 반환 불가 → bin에 등록해 재사용
  - 작은 조각 → tcache / fastbin (병합 안 함)
  - 나머지 → 일반 bin (이웃과 병합, 병합 결과가 top에 닿으면 top에 흡수)
- **malloc 탐색 순서**: tcache → fastbin → 일반 bin → top chunk 잘라 쓰기 → sbrk로 힙 확장
- **mallinfo와의 대응**: `arena`=힙 전체, `uordblks`=사용 중, `fordblks`=빈 공간 전체, `fsmblks`=fastbin 몫, `keepcost`=top chunk, `hblkhd`=mmap 영역
- "free 많이 했는데 RSS가 안 줄어요" → 빈 공간이 중간에 흩어져 있어서. `malloc_trim(0)`을 호출하면 중간의 빈 페이지도 OS에 돌려주려고 시도합니다.

## 11. fastbin과 일반 bin을 분리하는 이유

**속도와 단편화 방지라는 충돌하는 목표를 한 구조로 동시에 만족시킬 수 없어서**입니다.

- **일반 bin**: free 시 이웃과 병합해 단편화를 막음. 중간에서 빼야 하므로 이중 연결 리스트 필요. 메모리 효율은 좋지만 작업이 많음.
- **작은 객체의 현실**: 자주 만들고 금방 버리며, 같은 크기를 곧 다시 요청함. 일반 bin 방식이면 "병합했다가 → 바로 다시 쪼개는" 헛수고가 반복됨.
- **fastbin의 해법**: 병합을 미루고 그대로 재사용
  - 단일 연결 + LIFO → O(1)
  - 단순한 구조라 원자적 연산(CAS)으로 락 비용 절감
  - LIFO라 CPU 캐시에 남아 있을 메모리를 재사용하는 효과
- **대가(단편화)의 처리**: 병합을 포기하지 않고 **미뤄둠(deferred coalescing)**. 큰 요청이 오는 등 필요한 시점에 `malloc_consolidate()`로 한꺼번에 병합.
- tcache(2.26+)는 같은 발상을 스레드별로 확장해 아레나 락조차 안 잡게 만든 구조입니다.

## 12. fastbin에 크기 제한이 있나요?

있습니다. 두 겹의 크기 제한이 있고, 개수 제한은 없습니다.

**① 기본 한도 (실제 적용)**: `M_MXFAST` 기본값 = 64×sizeof(size_t)/4 → 64비트 128. 헤더와 16바이트 정렬을 고려하면 **chunk 0x80까지, 요청 크기 약 120바이트(0x78) 이하**가 대상입니다.

| 요청 크기 (64비트) | chunk 크기 | fastbin 인덱스 |
|---|---|---|
| 1 ~ 24 | 0x20 | 0 |
| 25 ~ 40 | 0x30 | 1 |
| … | … | … |
| 105 ~ 120 | 0x80 | 6 |

32비트는 기본 한도 64바이트, 단위 8바이트.

**② 구조상 최대**: `MAX_FAST_SIZE` = 80×SIZE_SZ/4 → 64비트 160. 배열은 10칸(`NFASTBINS`)이지만 기본 설정에선 앞 7칸만 사용. `mallopt(M_MXFAST, 160)`으로 올려야 뒤 칸도 쓰임. 0이면 fastbin 끔.

- man page BUGS: `M_MXFAST`는 계산 오류로 정확히 적용 안 될 수 있으므로 (2k+1)×sizeof(size_t)의 다음 배수로 올림해서 줄 것.

**③ 개수 제한 없음**: tcache는 bin당 7개 제한이 있지만 fastbin은 무제한. 대량 free 시 tcache를 채우고 남은 게 fastbin에 계속 쌓임 → `smblks`, `fsmblks`로 확인 가능.

## 13. 일반 bin 구분은 glibc 몇 버전 이야기인가요?

특정 버전이 아니라 **처음부터 있던 기본 구조**이며 최신 버전까지 유지됩니다.

- 계보: dlmalloc(Doug Lea) → ptmalloc → ptmalloc2(dlmalloc 2.7.x에서 포크) → glibc가 자체 수정
- unsorted/small/large 구분은 dlmalloc 2.7.x 시절부터 있던 설계. glibc에 ptmalloc2가 들어간 2000년대 초반부터 뼈대가 같음

**버전별로 바뀐 부분**

| glibc 버전 | 변경 사항 |
|---|---|
| 2.26 (2017) | tcache 추가 → 작은 할당은 tcache → fastbin → 일반 bin 순서 |
| 2.26 | x86(32비트)도 16바이트 정렬 → bin별 크기 계산 달라짐 |
| 2.32 | safe-linking: tcache/fastbin의 next 포인터 난독화 |
| 2.33 | `mallinfo2()` 추가 |

2.25 이하 자료에서는 작은 chunk가 바로 fastbin으로 간다고 설명되어 있을 수 있습니다.

## 14. small bin / large bin이 뭔가요?

**신발 가게 창고 비유**

- **small bin**: 자주 찾는 사이즈마다 전용 칸. "240 주세요" → 그 칸 맨 앞 것을 집어주면 끝.
- **large bin**: 큰 물건은 1mm 단위로 칸을 만들면 칸이 수백만 개 필요 → 범위로 묶은 칸을 두고 칸 안에서 크기순 정렬. 딱 맞거나 가장 비슷한 것을 찾아 크면 잘라서 줌.

```
small bin: [0x20][0x30][0x40] … [0x3f0]     칸 하나 = 정확히 한 크기 → 탐색 없음
large bin: [0x400~0x43f][0xc00~0xdff] … [나머지 전부]
                                             칸 하나 = 크기 범위 → 정렬·탐색
```

**실제 기준 (64비트)**

- chunk < 0x400(1024B) → small bin. 16바이트 간격 62칸 (0x20, 0x30 … 0x3f0)
- chunk ≥ 0x400 → large bin. 범위가 점점 넓어짐: 64B 범위 32칸 → 512B 범위 16칸 → 4096B 범위 8칸 → 32768B 범위 4칸 → 262144B 범위 2칸 → 나머지 1칸

**왜 나눴나**: 작은 할당은 압도적으로 많고 크기 종류가 적어 1:1 칸이 효율적. 큰 할당은 드물지만 범위가 넓어서 범위로 묶고 정렬 비용을 감수.

**unsorted bin과의 흐름**

1. free → tcache/fastbin에 못 들어가면 이웃과 병합
2. unsorted bin(임시 대기칸)에 넣음
3. 다음 malloc 때 unsorted bin을 훑으며 딱 맞으면 바로 반환
4. 아닌 것들은 그때 크기에 따라 small/large bin으로 분류

→ 반납함에 넣어뒀다가 다음 손님 올 때 선반에 정리하는 셈. 정렬 로직이 한 곳에만 있으면 되는 장점.

참고: fastbin 크기(0x20~0x80)는 small bin 범위와 겹칩니다. fastbin에 있던 chunk도 `malloc_consolidate()`로 정리되면 병합 후 unsorted를 거쳐 small/large bin으로 갈 수 있습니다.

---

## 15. 참고 링크 모음

**man page**

- mallopt(3): https://man7.org/linux/man-pages/man3/mallopt.3.html
- mallinfo(3): https://www.man7.org/linux/man-pages/man3/mallinfo.3.html
- mallinfo(3) (mankier): https://www.mankier.com/3/mallinfo
- credentials(7): https://man7.org/linux/man-pages/man7/credentials.7.html
- setuid(2): https://man7.org/linux/man-pages/./man2/setuid.2.html
- 환경 변수 적용 시점 설명: https://unix.com/man-page/xfree86/3/mallopt

**glibc 공식**

- Memory Allocation Tunables: https://gnu.org/software/libc/manual/html_node/Memory-Allocation-Tunables.html
- Tunables (sourceware 최신판): https://sourceware.org/glibc/manual/latest/html_node/Memory-Allocation-Tunables.html
- MallocInternals 위키: https://sourceware.org/glibc/wiki/MallocInternals
- malloc.c 소스: https://github.com/bminor/glibc/blob/a07e000e82cb71238259e674529c37c12dc7d423/malloc/malloc.c#L1711

**힙 구조 해설**

- HackTricks Bins & Memory Allocations: https://book.hacktricks.wiki/en/binary-exploitation/libc-heap/bins-and-memory-allocations.html
- HackTricks GitBook 미러: https://angelica.gitbook.io/hacktricks/binary-exploitation/libc-heap/bins-and-memory-allocations
- how2heap heap basics: https://www.mintlify.com/shellphish/how2heap/fundamentals/heap-basics
- how2heap 용어집: https://www.mintlify.com/shellphish/how2heap/reference/glossary
- Lazenca ptmalloc2 설명 (한국어): https://www.lazenca.net/pages/viewpage.action?pageId=51970061
- NCC Group dlmalloc/ptmalloc 버전 관계: https://www.nccgroup.com/research/cisco-asa-series-part-four-dlmalloc-28x-libdlmalloc-dlmalloc-on-cisco-asa/
- gmcalc (버전별 bin 크기 계산기): https://github.com/hackliza/gmcalc
- Privileged Programs 발표 자료 (Michael Kerrisk): https://www.man7.org/conf/meetup/privileged-programs--jambit-Kerrisk-2019-04-01.pdf

**기타**

- Android bionic malloc.h: https://android.googlesource.com/platform/bionic/+/44f0faa24/libc/include/malloc.h
