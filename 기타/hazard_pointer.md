# hazard pointer

태그드 포인터는 하드웨어 종속적임 ->ARM, 32bit 등 제약사항있음

### 컨셉
메모리를 바로 해제하지 않고 충분히 긴 시간 후 해제
hazard pointer를 가지고 있는 리스트
쓰레드 별 해제 대상을 저장하는 리스트

```
// 전역 Hazard Pointer 테이블
static hazard_slot_t hazard_table[MAX_THREADS][MAX_HAZARDS_PER_THREAD];

// 스레드별 Retired List
typedef struct retired_node {
    void *ptr;
    struct retired_node *next;
} retired_node_t;
```

### 알고리즘
- 주소에 접근하기 전에 하자드 포인트 리스트에 등록
- 사용 끝나면 하자드 포인트 리스트에서 제거
- 삭제 필요시
    - 하지드 포인터 리스트에 없으면 삭제
    - 있으면 리타이어 리스트에 넣었다가 다음번에 삭제


### 주의할점
- 하자드 포인터 리스트 오버 플로우
- 해시맵 / 트리 (랜덤 접근 + 중간 삭제가 많은 구조) → Hazard Pointer가 더 적합.
- 순차적 접근이라면 다른 방법 고려 필요