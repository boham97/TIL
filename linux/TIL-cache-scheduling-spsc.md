# TIL — 캐시 계층, 스케줄링, False Sharing, 락프리 큐

## 메모리 계층 — 사이클/지연 표

| 레벨 | 지연(대략) | CPU 사이클(3GHz) | 크기 | 공유 범위 |
|---|---|---|---|---|
| 레지스터 | < 1ns | 1 | 수십~수백 B | 코어(스레드) |
| **L1** | ~1ns | **~4** | 32~64KB | 코어 전용 (D/I 분리) |
| **L2** | ~3ns | **~12** | 256KB~1MB | 코어 전용 |
| **L3 (LLC)** | ~10ns | **~40** | 수~수십 MB | **소켓 공유** |
| **RAM** | ~80ns | **~240** | 수~수백 GB | 전역 |
| (참고) SSD | ~100μs | ~300,000 | — | — |

> RAM 한 번 = L1 **60번**. 이게 캐시가 필수인 이유.
> working set이 L1→L2→L3→RAM 경계를 넘을 때마다 벤치마크가 **계단식으로 느려진다.**

핵심: **L1/L2는 코어 전용, L3는 소켓 공유, RAM은 전역.** 데이터는 항상 **캐시라인(64B) 단위**로 이동.

## False Sharing
- 논리적으로 독립된 변수가 **같은 64B 라인**에 있어서, 두 코어가 각자 write하면 MESI가 라인 단위로 동작해 **무한 핑퐁** → lock 없는데도 수십 배 느려짐.
- 해결: `alignas(64)`로 라인 분리.
- **공유 안 하거나 read-only면 정렬 불필요** (오버킬, 메모리 낭비). 유일한 기준은 "**다른 코어가 동시에 write하는가**".
- 단, SIMD 정렬·큰 구조체의 라인 경계 가로지르기는 false sharing과 무관하게 정렬이 이득일 수 있음.

## CPU 스케줄링 × 캐시

| 스케줄링 결정 | 결과 |
|---|---|
| 같은 코어 유지 | L1/L2 **warm**, 빠름 |
| 코어 이동(같은 L3) | L1/L2 cold, L3 hit 가능 |
| **소켓 이동(NUMA)** | 전부 cold + 원격 메모리(~1.5~3배), false sharing 페널티 **폭증** |

스케줄러는 cache affinity를 지키려 마이그레이션을 꺼림 (Linux CFS, NUMA 도메인).

## 스레드 > 코어
- 캐시는 스위치 때 **비워지지 않음** — 새 스레드가 **덮어쓸 뿐**. 결과적으로 cold.
- 진짜 비용은 직접 비용(1~10μs)이 아니라 **간접 비용(수십~수백μs)**: L1/L2 cold, TLB miss, 분기예측기·프리페처 reset.

| 워크로드 | 권장 스레드 수 |
|---|---|
| CPU-bound | 물리/논리 코어 수 |
| I/O-bound (블로킹) | 코어 수보다 훨씬 많이 |
| I/O-bound (async) | 코어 수 ± 1 (이벤트 루프) |
| 혼합 | 코어 수 × 1~2배에서 벤치마크 |

- 같은 프로세스 스레드 전환 ≪ 다른 프로세스 전환 (페이지테이블·TLB 차이). PCID/ASID로 TLB flush 완화.

## async가 I/O에서 캐시 친화적인 이유
- 핵심은 **OS 컨텍스트 스위치 횟수를 압도적으로 줄이는 것** → L1/L2/TLB/분기예측기가 hot 유지.
- 유저 공간 스위치(ns) vs 커널 스위치(μs)로 자릿수가 다름.
- 단, hot한 건 **이벤트 루프 코드 path**지 각 요청의 **데이터**가 아님. work-stealing 런타임(Go/Tokio)은 태스크 마이그레이션 발생.

## SPSC 락프리 큐 — 실전 패턴

```cpp
template<typename T, size_t N>
struct SpscQueue {
    alignas(64) std::atomic<size_t> head;     // consumer만 write
    alignas(64) size_t cached_tail;           // consumer의 로컬 복사본

    alignas(64) std::atomic<size_t> tail;     // producer만 write
    alignas(64) size_t cached_head;           // producer의 로컬 복사본

    alignas(64) T buffer[N];
};
```

세 가지가 **상호 보완** (대체 아님):

| 기법 | 막는 문제 |
|---|---|
| `head`/`tail` **별도 라인 정렬** | producer↔consumer false sharing |
| `cached_head`/`cached_tail` **로컬 인덱스** | 다른 코어 소유 라인 가져오는 횟수 자체 |
| **pinning** (`sched_setaffinity`) | 마이그레이션·cache cold |

- `cached_*` 트릭: producer는 매번 `head`를 atomic load 하지 않고 자기 복사본을 봄. 가득 찼는지 의심될 때만 진짜 `head`를 읽어 갱신 → **cross-core traffic 감소**.
- pinning은 **같은 소켓 내 권장** (L3 공유, ~40 cycle). 다른 소켓이면 인터커넥트 경유 ~200 cycle.
- `cacheline_aligned`는 마이그레이션을 막진 못하지만, 마이그레이션이 일어나면 막아야 할 핑퐁이 더 비싸지므로 **정렬의 가치가 오히려 커진다(보험 역할)**.

### 근거 자료
- Erik Rigtorp — https://rigtorp.se/ringbuffer/ , https://github.com/rigtorp/SPSCQueue (캐시 인덱스 패턴의 가장 또렷한 출처)
- Facebook Folly `ProducerConsumerQueue.h` — https://github.com/facebook/folly/blob/main/folly/ProducerConsumerQueue.h
- DPDK `rte_ring` — https://doc.dpdk.org/guides/prog_guide/ring_lib.html
- LMAX Disruptor 백서 — https://lmax-exchange.github.io/disruptor/
- Lê, Pop, Cohen, Nardelli, *Correct and Efficient Bounded FIFO Queues* (PPoPP 2013) — https://doi.org/10.1145/2442516.2442527
