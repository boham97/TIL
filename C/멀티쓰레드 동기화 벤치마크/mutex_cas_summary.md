# 멀티스레드 동기화 벤치마크 정리

## 1. 기본 개념

### 시스템 콜이란?
사용자 프로그램이 커널(운영체제)에게 무언가를 해달라고 요청하는 인터페이스.

- **User space**: 일반 프로그램이 실행되는 영역. 하드웨어 직접 접근 불가.
- **Kernel space**: 운영체제가 실행되는 영역. 하드웨어 직접 제어 가능.

`qsort`, `memcpy` 같은 함수는 순수하게 user space에서 동작 → **시스템 콜 없음(0개)**

---

## 2. bench.c 개요

멀티스레드 동기화 방식 두 가지를 비교하는 벤치마크 프로그램.

> 생산자 스레드 N개가 정수 데이터를 집어넣고, 소비자 스레드 N개가 꺼내 먹는다.  
> 이 과정을 MUTEX 방식과 CAS 방식으로 각각 구현해 성능을 비교한다.

생산 데이터는 `id * 1000 + i` 형태의 정수. 내용보다 **동기화 메커니즘 자체의 성능**을 측정하는 것이 목적.

---

## 3. 두 가지 동기화 방식

### MUTEX 방식 (뮤텍스 + 조건변수)
- 자료구조: 원형 큐 (ring buffer)
- 동기화: `pthread_mutex` + `pthread_cond_wait`
- 락을 잡지 못하면 커널에 "나 재워줘" → **futex 시스템 콜 발생**
- 락이 풀리면 커널이 깨워줌 → **컨텍스트 스위치 발생**

### CAS 방식 (lock-free 스택)
- 자료구조: 연결 리스트 스택
- 동기화: `atomic_compare_exchange_weak` (CPU 원자 명령어 LOCK CMPXCHG)
- 커널 개입 없이 **user space에서 처리**
- 충돌 시 재시도(스핀) → CPU를 양보하지 않고 바쁘게 대기

---

## 4. 주요 버그 및 해결 과정

### 버그 1: cas_consumer에서 double pop
```c
// 잘못된 코드
while (!atomic_load(&cas_done) || cas_pop(&cs, &val)) {  // 여기서 pop
    if (cas_pop(&cs, &val))                               // 여기서 또 pop
        consumed++;
}
```
while 조건에서 이미 노드를 꺼내고, 본문에서 또 꺼내는 버그 → `double free` 발생

**수정:**
```c
while (1) {
    if (cas_pop(&cs, &val)) {
        consumed++;
    } else if (atomic_load(&cas_done)) {
        break;
    }
}
```

---

### 버그 2: SIGSEGV (ABA 문제 / use-after-free)
```
#0  cas_pop ()
#1  cas_consumer ()
```

**원인:** `malloc/free` 기반 lock-free 자료구조의 고전적인 문제.
```
old = atomic_load(&top)   // old 포인터 읽음
                          // ← 다른 스레드가 old를 pop하고 free()
next = old->next          // ← 이미 해제된 메모리 접근 → SIGSEGV
```

**해결: 노드 풀(pre-alloc)**  
`malloc/free`를 제거하고 미리 할당한 배열에서 노드를 꺼내 씀.
```c
#define POOL_SIZE (NUM_PRODUCERS * ITEMS_PER_PRODUCER + 16)
static Node node_pool[POOL_SIZE];
static atomic_int pool_idx;

Node *pool_alloc(void) {
    int idx = atomic_fetch_add(&pool_idx, 1);
    return &node_pool[idx];
}
```
벤치마크 종료 시 풀째로 버리므로 free 불필요.  
메모리 사용량: 약 **192MB** (8 * 1000000 노드).

---

## 5. strace 분석

### -f 플래그의 중요성

멀티스레드 프로그램은 반드시 `-f`를 붙여야 worker 스레드까지 추적된다.

```
strace -c  (메인 스레드만): MUTEX futex 4회,  CAS futex 4회  → 차이가 안 보임
strace -cf (전체 스레드):   MUTEX futex 663만회, CAS futex 3회 → 진짜 병목 드러남
```

---

### strace -cf 결과 비교

| 항목 | MUTEX | CAS |
|---|---|---|
| 총 실행 시간 | 105,676 ms | 3,310 ms |
| futex 호출 수 | **6,638,766회** | 3회 |
| futex 에러 수 | 1,553,682회 | 0회 |
| futex 총 소요 | 834.4초 | 0.144초 |
| 전체 시스템 콜 | 6,638,905회 | 143회 |

---

### futex 호출이 생산/소비 합계보다 적은 이유

```
생산 + 소비 합계: 16,000,000회
futex 호출:        6,638,766회  ← 더 적다
```

`pthread_mutex_lock`은 항상 futex를 호출하지 않는다.

```
락 경합 없음 → user space에서 atomic 명령어로 바로 해결 → futex 없음
락 경합 있음 → 커널에 "나 재워줘"                       → futex 발생
```

즉 futex 663만 회는 **경합이 발생한 횟수**만 카운트된 것이다.  
16,000,000번 중 663만 번은 커널을 거쳤고 나머지는 user space에서 해결했는데,  
그 663만 번이 834초를 만들어냈다.

### futex 에러 1,553,682회의 의미

`cond_wait`에서 깨어났는데 조건이 아직 안 맞아서 **다시 자러 간 횟수**.  
spurious wakeup 또는 경합 패배가 원인.

### CAS가 빠른 이유

futex 3회는 스레드 생성/종료 때만 발생.  
실제 push/pop **8,000,000번은 futex 단 한 번도 없이** CPU 명령어만으로 처리됨.

---

## 6. 언제 뭘 쓸까?

### 뮤텍스가 유리한 경우
- 스레드 수가 적을 때 (경합 자체가 적음)
- 작업 시간이 길 때 (CPU를 스핀으로 낭비하는 것보다 재우는 게 나음)
- 코드 유지보수가 중요할 때 (훨씬 직관적)

### CAS가 유리한 경우
- 스레드가 매우 많고 경합이 심할 때
- 작업이 매우 짧고 빠를 때
- 레이턴시가 극도로 중요할 때 (게임 서버, 고빈도 거래 등)

### 실무 정석
> 일단 뮤텍스로 짜고, 프로파일링해서 병목이 확인되면 그때 CAS나 lock-free로 바꾼다.

---

## 7. 컴파일 & 실행

```bash
gcc -std=gnu99 -O2 -o bench bench.c -lpthread

./bench mutex   # 뮤텍스만
./bench cas     # CAS만
./bench all     # 둘 다 비교

# strace로 시스템 콜 분석 (멀티스레드는 반드시 -f)
strace -cf ./bench mutex
strace -cf ./bench cas

# gdb로 디버깅
gdb --args ./bench all
(gdb) run
(gdb) bt          # 스택 트레이스
(gdb) info threads
```
