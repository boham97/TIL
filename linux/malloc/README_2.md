# calloc / brk / mmap 실험 기록

세션 재시작해도 맥락 이어갈 수 있도록 지금까지 한 실험과 결론을 정리.

## 배경 질문

"calloc 후 데이터 쓰고 다시 할당하면 RSS가 증가하는지" 테스트하다가
glibc malloc 내부의 brk↔mmap 전환, 동적 mmap_threshold 조정("래칫"),
tcache 재사용, calloc의 zero-fill 보장 메커니즘까지 확인하게 됨.

현재 `main.c`는 마지막 실험(128KB 경계 정밀 테스트) 코드 상태임.
아래 각 실험은 `main.c`를 그때그때 다르게 고쳐가며 진행한 것이라,
재현하려면 각 절의 코드 스니펫으로 바꿔서 다시 컴파일해야 함
(`gcc -O0 -g -o a.out main.c`).

---

## 실험 1: 고정 크기 vs 가변 크기 반복 calloc (4MB대)

50번 반복해서 `calloc(1, sz)` → `memset` 4096바이트 → `free`.
- 고정 모드: `sz`가 항상 4MB로 동일
- vary 모드 (`argv[1]` 아무거나): `sz = 4MB + (i%17)*64KB`로 매번 다름

**결과**
- 고정 크기: 첫 alloc만 mmap(4MB > 128KB threshold라서), 이후부턴 free된 chunk의
  크기(≈threshold)가 threshold를 상향시켜서 두 번째 alloc부터 brk로 전환, 포인터
  고정, RSS 평평(5316KB).
- 가변 크기: `i=0~16`은 매번 새 크기라 매번 mmap/munmap, RSS 평평(1420KB, OS에
  즉시 반납되니까). `i=16`에서 가장 큰 청크(~5.2MB)를 free하는 순간 glibc가
  mmap_threshold를 그 크기로 올려버려서, `i=17`부터는 이후 요청(4~5MB대)이 새
  threshold보다 작아져 brk로 빠짐. brk 힙은 자동 축소가 안 되니 RSS가
  5572→5836→6100→6364KB로 **계단식 증가** 후 정체.

**strace로 확인**: `strace -e trace=brk,mmap,munmap`로 정확히 위 전환 시점을
brk()/mmap()/munmap() 호출 순서로 확인함.

**검증**: `GLIBC_TUNABLES=glibc.malloc.mmap_threshold=131072`로 threshold를
고정하면 동적 상향(래칫)이 꺼져서, vary 모드에서도 50번 내내 mmap↔munmap만
반복하고 RSS가 끝까지 1540KB로 평평해짐 → **원인이 "동적 mmap_threshold
래칫"이라는 게 확정됨**.

---

## 실험 2: 작은 크기(4KB) 반복 — 왜 매번 같은 주소가 나오나

`base=4KB`로 줄여서 반복 테스트하니 포인터가 계속 동일 → 질문 발생:
"memset으로 오염시켰는데 왜 같은 주소를 또 주지? calloc이 0을 보장하는데
다른 주소를 주고 나중에 반납할 줄 알았다."

**답**
1. 4KB는 mmap_threshold(128KB)보다 훨씬 작아서 애초에 brk 힙 경로만 탐.
2. `free()`는 이 크기에서 OS에 반납하지 않고 유저공간 free list(tcache 등)에만
   넣어둠 — 힙 페이지 자체는 프로세스가 계속 보유.
3. 한 번에 살아있는 청크가 1개뿐이라 free→calloc이 LIFO로 정확히 같은 슬롯을
   재사용 → 주소 동일.
4. calloc의 zero-fill 보장은 "커널이 갓 준 새 페이지는 원래 0"이라는 사실에
   기댄 최적화일 뿐. **재활용된 청크**를 내줄 땐 glibc가 내부적으로 명시적
   `memset(0)`을 다시 수행함. 별도 테스트(`zero_check.c`, scratchpad에 있었음)로
   확인: 직전에 `memset(p,1,...)`로 오염시켜도 다음 calloc의 첫 바이트는 항상 0.
5. strace로도 반복 구간 동안 brk/mmap/munmap이 전혀 안 나가는 걸 확인 —
   순수 유저 공간 재사용임을 뒷받침.

---

## 실험 3: 128KB 경계 정밀 스윕 (현재 `main.c` 상태)

`131072-64`부터 `131072+64`까지 8바이트 간격으로 `calloc`→`memset`→`free`→
`malloc_trim(0)` 반복. (`malloc_trim`이 없으면 free된 chunk가 top chunk에
합쳐져서 다음 요청이 sysmalloc/mmap 판단 로직 자체를 안 타므로, threshold를
넘겨도 계속 brk로 착각하게 됨 — 처음에 이걸로 한 번 낚였음.)

**glibc의 실제 chunk 크기 계산**: `nb = (req + 8 + 15) & ~15`

| 요청 크기 | nb |
|---|---|
| 131048 | 131056 |
| **131056** | **131072** ← 기본 threshold(131072)와 최초로 같아짐 |
| 131064 | 131072 |
| 131072 | 131088 |

**기본(동적 threshold) 상태** (`strace_boundary.txt`, `prog_boundary.txt`):
`size=131056`에서 딱 한 번 mmap 발생(`mmap(NULL, 135168, ...)`, 페이지 정렬
때문에 135168바이트로 잡힘). 곧바로 free되면서 "방금 munmap한 크기(135168) >
현재 threshold(131072)"라 threshold가 **즉시 135168로 상향**됨. 그 결과 바로
다음 요청부터(`131072 < 135168`) 다시 brk로 떨어지고, 테스트 범위 끝(nb=131152)
까지도 135168을 못 넘어서 mmap이 두 번 다시 안 나옴. → 경계 근처에서도 똑같이
"동적 threshold 래칫"이 작동함을 확인.

**threshold 고정 상태** (`strace_boundary_fixed.txt`, `prog_boundary_fixed.txt`,
`GLIBC_TUNABLES=glibc.malloc.mmap_threshold=131072`): 래칫이 꺼지니
`size=131056`(nb=131072)에서 정확히 brk→mmap 전환 후, 나머지 전 구간
(131064~131136)이 계속 mmap 유지. 포인터도 `0x1fb48c0`(brk)에서
`0x7ff4e6551010`(mmap)로 딱 한 번 바뀌고 끝까지 유지됨.

**결론**: 128KB 경계는 "요청 바이트 수"가 아니라 glibc가 16바이트 정렬로 계산한
**chunk 크기(nb)** 기준이고, 정확한 크로싱 지점은 요청 131056바이트(nb=131072)
였음. 이 경계를 넘는 mmap/munmap 한 번만으로도 페이지 라운딩 때문에 threshold가
즉시 재조정되므로, 반복적인 크로싱을 관찰하려면 threshold를 고정해야 함.

---

## 실험 4: 128KB 미만에서 크기를 계속 바꿔도 mmap이 뜨는가

`calloc` 크기를 100바이트~127KB 사이에서 매번 다르게(20회, `sz = 100 +
(i*40009) % (threshold-108)`) 반복하며 `calloc`→`memset`→`free`→
`malloc_trim(0)`. (`below_threshold.c`)

**프로그램 출력**: `i=1`부터 포인터가 `0x1738920`으로 끝까지 고정, RSS도
1464KB로 평평.

**strace로 확인** (`strace_below_threshold.txt`): 로더(ld.so)가 자기 자신을
매핑하느라 나는 초기 mmap/munmap 몇 번을 빼면, 20번의 루프 동안 **mmap/munmap이
전혀 발생하지 않음** — 전부 `brk()`만 오르내림. `brk()` 주소가 요청 크기에 따라
`0x96d000`~`0x9ac000` 사이를 오르내리는 게 보이는데, 큰 요청이 들어오면 top
chunk를 늘리려 brk를 밀고, 바로 다음 `free`+`malloc_trim(0)`이 다시 낮춘 것.

**결론**: 크기가 7KB~127KB로 아무리 들쭉날쭉해도 nb가 threshold(131072)를 못
넘으면 mmap 경로 자체를 안 탄다 — "동적 threshold 래칫"과 별개로, brk/mmap
분기는 순수하게 **매 요청의 nb 대 threshold 비교**로만 결정됨. calloc이
돌려주는 유저 포인터는 청크 헤더 위치라 재사용되는 한 고정되고, 대신 top
chunk 경계(=brk 값)만 요청 크기에 맞춰 늘었다 줄었다 한다.

**부록 — brk 힙은 언제 늘어나나**: `brk()`(sbrk) syscall은 아래 순서로 찾아도
요청을 못 채울 때만 호출되는 최후 수단이다.
1. tcache/fastbin/smallbin 같은 유저공간 free list에서 맞는 청크를 찾으면
   그걸 내주고 끝 (syscall 없음)
2. free list에 없으면 힙 끝의 top(wilderness) chunk를 잘라 씀 (역시 syscall
   없음)
3. top chunk도 모자라야 비로소 `brk()`로 program break를 밀어 top chunk
   자체를 키움

이번 실험은 `malloc_trim(0)`으로 매 free 직후 top chunk를 최소치까지 깎아
버렸기 때문에 조금만 커도 매번 3번 케이스가 강제로 재현된 것. trim을 안 하면
free된 공간이 top에 병합되어 남으므로, 다음 요청이 이전 최대치보다 작으면
brk를 다시 부를 필요가 없다 (실험 3에서 처음에 낚였던 것과 같은 이유).

---

## brk vs mmap 개념 정리

| | brk (힙) | mmap |
|---|---|---|
| 구조 | 프로세스에 하나뿐인 연속 힙의 끝(program break)을 밀고 당김 | 독립된 새 매핑 영역을 그때그때 생성 |
| 언제 쓰나 (glibc 기본) | 작은 요청 (< 128KB, `mmap_threshold`) | 큰 요청 (≥ 128KB) |
| free 시 OS 반납 | 즉시 안 됨. 중간 chunk는 free list 재활용만, 맨 끝(top)만 축소 가능 (`malloc_trim`) | `free()` 즉시 `munmap`으로 완전 반납 |
| 속도 | 빠름 (유저공간 free list 재사용, 시스템콜 없음) | 느림 (매번 커널 페이지 테이블 조작) |
| 단편화 | 생길 수 있음 → 계단식 RSS 증가의 원인 | 안 생김 (독립 영역 통째로 반납) |
| 주소 특징 | 낮은/고정된 힙 영역 주소 | 높고 매번 다른 주소 (`0x7f...`) |

정리: 작고 자주 쓰는 메모리는 brk 힙에서 빠르게 재활용하고, 크고 어쩌다 쓰는
메모리는 mmap으로 받아서 쓰자마자 확실히 반납하는 게 glibc 설계 의도. 지금까지
본 모든 현상(계단식 RSS 증가, 같은 주소 재사용, threshold 래칫)은 전부 이 두
경로 사이의 전환 로직에서 나온 것.

---

## 파일 구성

- `main.c` — 128KB 경계 스윕 테스트 코드 (실험 3, 최신 상태)
- `a.out` — `main.c` 빌드 산출물
- `prog_boundary.txt` / `strace_boundary.txt` — 기본(동적 threshold) 상태 결과
- `prog_boundary_fixed.txt` / `strace_boundary_fixed.txt` — threshold 고정
  (`GLIBC_TUNABLES=glibc.malloc.mmap_threshold=131072`) 상태 결과
- `below_threshold.c` — 128KB 미만 가변 크기 반복 테스트 코드 (실험 4)
- `strace_below_threshold.txt` — 실험 4의 strace 결과

## 재현 명령어 모음

```bash
# 빌드
gcc -O0 -g -o a.out main.c

# 기본 실행 + strace
./a.out > prog_boundary.txt
strace -f -tt -e trace=brk,mmap,munmap -o strace_boundary.txt ./a.out > /dev/null

# threshold 고정 실행 + strace
GLIBC_TUNABLES=glibc.malloc.mmap_threshold=131072 ./a.out > prog_boundary_fixed.txt
GLIBC_TUNABLES=glibc.malloc.mmap_threshold=131072 \
  strace -f -tt -e trace=brk,mmap,munmap -o strace_boundary_fixed.txt ./a.out > /dev/null

# 실험 4 (128KB 미만 가변 크기)
gcc -O0 -g -o below_threshold below_threshold.c
./below_threshold
strace -f -tt -e trace=brk,mmap,munmap -o strace_below_threshold.txt ./below_threshold > /dev/null
```
