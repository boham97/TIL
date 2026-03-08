# Atomic 연산과 Memory Barrier 정리

## 왜 필요한가

CPU와 컴파일러는 성능을 위해 코드 실행 순서를 바꾼다.
싱글스레드에서는 문제가 없지만, 멀티스레드에서는 다른 스레드가 예상과 다른 순서로 값을 볼 수 있다.

```c
// 스레드 A
data = 42;       // (1)
ready = 1;       // (2)  ← CPU가 (1)보다 먼저 실행할 수 있음

// 스레드 B
if (ready == 1)  // ready는 1인데
    print(data); // data가 아직 42가 아닐 수 있음
```

순서를 바꾸는 주체는 두 곳이다:
- **컴파일러**: -O2, -O3 최적화 시 명령어 순서 재배치
- **CPU**: 파이프라인, store buffer, 비순차 실행(out-of-order execution)


## Memory Order 5단계

약한 순서 → 강한 순서

| memory order | 의미 | 용도 |
|---|---|---|
| `relaxed` | 원자성만 보장, 순서 보장 없음 | 카운터, 아직 나만 보는 데이터 |
| `acquire` | 이 읽기 이후의 연산이 위로 올라오지 못함 | load에 사용 |
| `release` | 이 쓰기 이전의 연산이 아래로 내려가지 못함 | store에 사용 |
| `acq_rel` | acquire + release 둘 다 | CAS처럼 읽기+쓰기 동시 연산 |
| `seq_cst` | 모든 스레드가 같은 순서로 봄 (전체 순서 보장) | 가장 안전, 가장 비쌈 |


## acquire / release 짝

```c
// 스레드 A (생산자)
data = 42;
atomic_store_explicit(&ready, 1, memory_order_release);
//   ↑ release: 이 위의 모든 쓰기(data=42)가 반드시 먼저 완료됨

// 스레드 B (소비자)
if (atomic_load_explicit(&ready, memory_order_acquire) == 1)
//   ↑ acquire: 이 아래의 모든 읽기(data)가 반드시 이후에 실행됨
    print(data);  // 42 보장
```

핵심 원칙:
- **다른 스레드의 쓰기를 읽을 때 → acquire**
- **다른 스레드에게 보여줄 쓰기를 할 때 → release**


## C11 Atomic 연산 목록

헤더: `#include <stdatomic.h>` (C11, `-std=c11` 이상)

### 읽기/쓰기
```c
atomic_load(&var);              // 원자적 읽기
atomic_store(&var, val);        // 원자적 쓰기
```

### 교환
```c
atomic_exchange(&var, new_val);                        // swap, 이전 값 반환
atomic_compare_exchange_strong(&var, &expected, new);  // CAS
atomic_compare_exchange_weak(&var, &expected, new);    // CAS, spurious failure 허용 (루프 안에서 더 빠를 수 있음)
```

### 산술
```c
atomic_fetch_add(&var, val);   // 더하고 이전 값 반환
atomic_fetch_sub(&var, val);   // 빼고 이전 값 반환
```

### 비트
```c
atomic_fetch_or(&var, val);
atomic_fetch_and(&var, val);
atomic_fetch_xor(&var, val);
```

### 펜스
```c
atomic_thread_fence(memory_order);  // 특정 순서의 barrier만 삽입
```

모든 연산에 `_explicit` 버전이 있다. 안 붙이면 기본 `seq_cst`.


## _explicit 차이

```c
// 기본: 항상 seq_cst (가장 비쌈)
atomic_load(&q->tail);

// explicit: 필요한 만큼만 지정
atomic_load_explicit(&q->tail, memory_order_acquire);
```


## __sync vs __atomic vs C11 atomic

| | `__sync` | `__atomic` | C11 `<stdatomic.h>` |
|---|---|---|---|
| 도입 | GCC 4.1 (2006) | GCC 4.7 (2012) | C11 표준 |
| memory order | 항상 full barrier (seq_cst) | 선택 가능 | 선택 가능 |
| 이식성 | GCC 전용 | GCC 전용 | 표준 (모든 C11 컴파일러) |
| C 표준 | 무관 (`-std=c89`도 가능) | 무관 | `-std=c11` 이상 |

```c
// __sync: 항상 full barrier, 선택지 없음
__sync_bool_compare_and_swap(&ptr, old, new);

// __atomic: memory order 선택 가능
__atomic_compare_exchange_n(&ptr, &old, new, false,
    __ATOMIC_ACQ_REL, __ATOMIC_RELAXED);

// C11: 표준, memory order 선택 가능
atomic_compare_exchange_strong_explicit(&ptr, &old, new,
    memory_order_acq_rel, memory_order_relaxed);
```


## x86에서 실제 CPU 명령어

| atomic 연산 | x86 명령어 |
|---|---|
| `atomic_load` (acquire) | `mov` (일반 load, x86은 load에 acquire 보장) |
| `atomic_store` (release) | `mov` (일반 store, x86은 store에 release 보장) |
| `atomic_store` (seq_cst) | `mov` + `mfence` 또는 `lock xchg` |
| CAS | `lock cmpxchg` |
| `atomic_fetch_add` | `lock xadd` |
| `atomic_exchange` | `lock xchg` |

x86은 기본적으로 load-load, store-store 순서를 보장하므로 acquire/release가 추가 비용 없이 동작한다.
seq_cst만 추가 fence(`mfence`)가 필요하므로 더 비싸다.

이것이 `__sync`(항상 seq_cst)에서 `_explicit`(acquire/release)로 바꾸면 빨라지는 이유다.


## Lock-Free 큐 적용 예시

```c
// 새 노드 초기화: 아직 나만 보는 데이터 → relaxed
atomic_store_explicit(&node->next, 0, memory_order_relaxed);

// 공유 변수 읽기: 다른 스레드가 쓴 값을 봐야 함 → acquire
old_tail = atomic_load_explicit(&q->tail, memory_order_acquire);

// CAS: 읽기(acquire) + 쓰기(release) 동시 필요 → acq_rel
// 실패 시: 어차피 retry → relaxed
atomic_compare_exchange_strong_explicit(&q->tail, &old_tail, new_tail,
    memory_order_acq_rel,    // 성공 시
    memory_order_relaxed);   // 실패 시
```



## 참고

- Michael & Scott, "Simple, Fast, and Practical Non-Blocking and Blocking Concurrent Queue Algorithms", PODC 1996
  - https://www.cs.rochester.edu/u/scott/papers/1996_PODC_queues.pdf
- GCC Atomic Builtins: https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html
- C11 stdatomic.h: https://en.cppreference.com/w/c/atomic
