# DPDK + 제로카피

## 핵심 한 줄 요약

> NIC이 DMA로 hugepage 메모리에 직접 쓰고, 앱이 그 주소를 구조체 포인터로 바로 읽는다. 복사 0번.

---

## 일반 소켓 vs DPDK + 제로카피

```
일반 소켓:
NIC → [커널 버퍼 복사] → [앱 버퍼 복사] → 구조체 캐스팅
      (커널 개입)         (memcpy)

DPDK + 제로카피:
NIC → DMA → hugepage 메모리 → 구조체 포인터
            (CPU/커널 개입 없음, 복사 없음)
```

---

## 구성 요소

### 1. DMA (Direct Memory Access)
- NIC이 CPU 개입 없이 직접 메모리에 패킷을 씀
- CPU는 "복사"가 아니라 "읽기"만 함

### 2. Hugepage
- 일반 메모리 페이지는 4KB → TLB 미스 잦음
- Hugepage는 2MB → TLB 미스 줄어들고 NIC DMA 타겟으로 적합
- DPDK가 hugepage 위에 메모리 풀(mbuf pool) 만들어서 관리

### 3. PMD (Poll Mode Driver)
- 커널 드라이버 대신 유저스페이스 드라이버
- 인터럽트 없이 NIC을 busy-poll로 계속 들여다봄
- 코어 피닝된 스레드가 전담

---

## 코드 흐름

```c
// 1. DPDK 초기화 - hugepage 위에 mbuf pool 생성
struct rte_mempool* mbuf_pool = rte_pktmbuf_pool_create(
    "MBUF_POOL", 8192, 256, 0,
    RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id()
);

// 2. NIC 포트 설정 - DMA 타겟을 mbuf pool로 지정
rte_eth_rx_queue_setup(port, 0, 128, rte_eth_dev_socket_id(port), NULL, mbuf_pool);

// 3. 수신 루프 - 커널 콜 없이 NIC 직접 폴링
struct rte_mbuf* pkts[32];
while (true) {
    uint16_t n = rte_eth_rx_burst(port, 0, pkts, 32);
    for (int i = 0; i < n; i++) {

        // 4. 제로카피 - NIC이 DMA로 쓴 메모리를 구조체로 바로 캐스팅
        uint8_t* raw = rte_pktmbuf_mtod(pkts[i], uint8_t*);
        MarketData* md = (MarketData*)(raw + UDP_HEADER_SIZE);

        // md->bid, md->ask 복사 없이 바로 접근
        process(md);

        // 5. mbuf 반환 - pool에 돌려줌 (free 아님)
        rte_pktmbuf_free(pkts[i]);
    }
}
```

---

## 메모리 구조

```
hugepage (2MB)
┌─────────────────────────────────┐
│  mbuf pool                      │
│  ┌───────┬───────┬───────┐      │
│  │ mbuf0 │ mbuf1 │ mbuf2 │ ...  │
│  └───────┴───────┴───────┘      │
│      ↑                          │
│   NIC DMA가 여기에 직접 씀       │
│   앱은 포인터로 바로 읽음        │
└─────────────────────────────────┘
```

---

## 복사 횟수 비교

| 방식 | 복사 횟수 | 커널 개입 |
|------|-----------|-----------|
| 일반 소켓 | 2번 | 있음 |
| 코드단 제로카피만 | 1번 | 있음 |
| DPDK만 | 1번 | 없음 |
| **DPDK + 제로카피** | **0번** | **없음** |

---

## 주의사항

- mbuf는 처리 즉시 `rte_pktmbuf_free()`로 pool에 반환해야 함
- 오래 들고 있으면 다음 패킷이 덮어씌워짐
- 오래 보관해야 하면 그때만 복사
- VM/도커 환경에서는 실제 NIC DMA 불가 → 성능 차이 안남 (실서버 필요)