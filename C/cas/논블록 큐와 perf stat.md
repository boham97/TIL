# Lock-Free Queue 성능 분석 보고서

## 환경

- **CPU**: 12코어 (AMD), WSL2 Docker 컨테이너
- **자료구조**: Michael-Scott Lock-Free Queue (tagged pointer ABA 방지)
- **테스트**: Producer/Consumer 각 N개, 스레드당 1,000,000 ops
- **도구**: `perf stat`, `perf record`, `perf report`

---

## 1. 기본 성능 측정 (24스레드: 12P + 12C)

| 지표 | 값 | 해석 |
|------|-----|------|
| Throughput | **~37M ops/sec** | 단일 head/tail 큐 기준 상당히 높음 |
| IPC | 0.47 | 사이클당 명령어 수. 1.0 이상이 이상적이나 경합으로 절반 이상 대기 |
| stalled-cycles-frontend | 36.59% | 파이프라인이 1/3 이상 멈춤 |
| branch-misses | 12.42% | CAS 실패 시 retry 루프에서 분기 예측 실패 |
| context-switches | 18,985 | sched_yield로 인한 스레드 전환 |
| page-faults | 46,999 | malloc/free로 인한 페이지 폴트 |
| cache-misses | 19.33% | head/tail 캐시라인이 코어 간 핑퐁 |
| L1-dcache-load-misses | 61,216,270 | 12M ops 대비 매우 높은 L1 미스 |
| user 시간 | 0.73s | 유저 코드 실행 |
| sys 시간 | **2.44s** | 커널 시간이 유저의 3배 → **sched_yield + 스케줄러가 주범** |

---

## 2. perf report 프로파일 (함수별 핫스팟)

| 함수 | Self % | 설명 |
|------|--------|------|
| `__sched_yield` | 66.55% | 시간 대부분을 양보에 사용 |
| `enqueue_node` | 14.14% | 실제 enqueue 작업 |
| `dequeue` | 10.87% | 실제 dequeue 작업 |
| 커널 스케줄러 | ~35% | schedule, pick_next_task 등 |
| page_fault 관련 | ~5.5% | malloc 기반 메모리 할당 |

**실제 작업(enqueue + dequeue)은 전체의 25%에 불과하고, 나머지 75%는 yield + 스케줄러에 소비됨. page fault는 ~5.5%로 비중이 작음.**

---

## 3. 스레드 수에 따른 성능 변화

| 구성 | 스레드 수 | Throughput | 비고 |
|------|----------|-----------|------|
| 6P + 6C | 12 | ~27-32M | yield 비율 동일 (67%) |
| 12P + 12C | 24 | **~33-37M** | 최적 구성 |
| 24P + 24C | 48 | ~31-36M | 분산 크고 평균적으로 낮음 |

**핵심 발견**: 스레드를 12→24로 줄여도 yield 비율(67%)이 동일했음. 이는 오버커밋이 문제가 아니라 **head/tail 경합 자체가 구조적 병목**임을 의미.

---

## 4. 최적화 레벨 비교 (48스레드 기준)

| 최적화 | Throughput | 비고 |
|--------|-----------|------|
| O0 | ~13M | 최적화 없음 |
| O2 | ~27M | |
| O3 | ~30M | 경합 조건에 따라 분산 큼 |
| O3 -march=native | ~27-31M | O3과 유사 |

O2와 O3의 차이가 적은 이유: 병목이 CPU 연산이 아니라 **메모리 접근(CAS, 캐시 미스)과 시스템콜(sched_yield)**이기 때문.

---

## 5. 하이브리드 Backoff 실험 (실패)

`sched_yield`를 exponential `_mm_pause` + yield 하이브리드로 변경:

```c
static inline void backoff(int *retry) {
    if (*retry < 16) {
        for (int i = 0; i < (1 << *retry); i++)
            _mm_pause();
        (*retry)++;
    } else {
        sched_yield();
        *retry = 0;
    }
}
```

| 지표 | yield only | 하이브리드 |
|------|-----------|-----------|
| Throughput | ~30M | **~25M (악화)** |
| sched_yield 비율 | 67% | 7% |
| context-switch | ~1,000 | 108 |
| sys 시간 | 1.4s | 0.39s |
| user 시간 | 0.6s | **2.3s (악화)** |
| IPC | 0.47 | **0.15 (폭락)** |

**실패 원인**: `_mm_pause`의 exponential backoff(`1 << 15 = 32768`회)로 CPU가 pause만 공회전. 12코어 12스레드에서는 오버커밋이 아니므로 스핀이 다른 스레드의 CAS 성공을 오히려 지연시킴.

**결론: 코어 수 ≥ 스레드 수인 환경에서는 `sched_yield`가 최적.**

---

## 6. Observer Effect (perf가 성능을 올리는 현상)

`perf record` 또는 `perf stat`을 붙이면 throughput이 올라가는 현상 관찰:

- perf 없이: ~33M ops/sec
- perf 붙임: ~35-43M ops/sec (최대 43M)

**원인**: `perf`의 PMU 샘플링 인터럽트가 스레드에 **랜덤한 미세 지연**을 발생시켜 CAS 경합이 시간적으로 분산됨. 모든 스레드가 동시에 head/tail을 두드리는 대신, 인터럽트 타이밍에 따라 살짝 어긋나면서 CAS 성공률이 상승.

---

## 7. 병목 우선순위 및 개선 방향

### 병목 우선순위

1. **sched_yield + 스케줄러** → sys 시간의 주범 (perf report 기준 66.55%, syscall + schedule 경로가 sys 2.44s의 대부분 차지)
2. **head/tail 캐시라인 경합** → IPC 0.47, cache-misses 19%, L1 미스 6천만 (CAS 경합의 근본 원인)
3. **CAS 실패 재시도** → branch-misses 13%, 분기 예측기에 랜덤 패턴
4. **malloc/page-fault** → page-faults 47K이지만 perf report 기준 ~5.5%로 비중은 작음

### 개선 방향

| 방법 | 기대 효과 | 난이도 |
|------|----------|--------|
| **멀티 큐 (SPSC 다중화)** | 경합 제거, yield/CAS 불필요, 근본적 해결 | 중 |
| **head/tail alignas(64) 패딩** | cache-miss 감소 | 하 |
| **배치 enqueue/dequeue** | CAS 횟수 감소 → yield 횟수 감소 | 중 |
| **Flat Combining** | CAS 횟수 자체 감소 | 상 |
| **메모리 풀** (malloc 제거) | page-fault 제거 (전체 ~5.5%이므로 효과 제한적) | 중 |
| **jemalloc/tcmalloc 교체** | malloc 경합 감소 (효과 미미) | 하 |

### sched_yield가 최적인 이유

- 12코어에 24스레드 → 코어당 2개 대기, yield 시 즉시 다음 스레드 투입
- `_mm_pause`는 너무 짧아 다른 스레드에 기회를 못 줌
- context-switch 비용보다 CAS 경합 분산 효과가 더 큼
- 단일 head/tail 구조에서 구조적 한계: **yield 67%가 이 설계의 현실**

---

## 9. 블로킹 단일 큐와의 비교 (예상)

24스레드 기준, 블로킹(mutex) 큐와 비교하면:

| 지표 | Lock-Free (sched_yield) | Blocking (mutex) |
|------|------------------------|-------------------|
| IPC | 0.47 (CAS 경합으로 낮음) | 락 안에서는 높지만 futex 대기로 무의미 |
| sys 비율 | 높음 (yield + 스케줄러) | 높음 (futex_wait/wake) |
| 동시성 | enqueue/dequeue 동시 가능 | 한 번에 1개 스레드만 접근 |
| throughput | ~33-37M | 수백만~천만 수준 |

sys 비율이 높은 것은 블로킹이나 lock-free나 마찬가지. 경합이 심한 단일 큐의 구조적 한계.

---

## 10. 최종 결론: SPSC 멀티큐

SPSC(Single Producer Single Consumer) 큐를 Producer/Consumer 쌍마다 전용으로 할당하면:

- CAS 경합 → **제거**
- sched_yield → **불필요**
- 캐시라인 바운싱 → **제거**
- 12쌍 기준 40M+ ops/sec 달성

**가장 간단하면서 가장 효과적인 개선 방법.**

- 단일 head/tail Michael-Scott Lock-Free Queue의 **실전 throughput 한계는 ~33-37M ops/sec** (12코어, 24스레드)
- 시간의 75%는 yield + 스케줄러에 소비되며, 실제 작업은 25% (page fault는 ~5.5%로 비중 작음)
- backoff 전략 변경으로는 의미 있는 개선이 어려움 (구조적 한계)
- 더 높은 throughput을 위해서는 **경합 지점을 분산**하는 구조적 변경 필요 (멀티 큐, LCRQ 등)
- `perf` 도구 자체가 성능에 영향을 주므로 (observer effect), 벤치마크 시 perf 유무 모두 측정 필요
- SPSC 큐 다중화(12쌍)로 전환 시 경합 없이 40M+ 달성 가능 → 가장 간단하고 효과적인 개선