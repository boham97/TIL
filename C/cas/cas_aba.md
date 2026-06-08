# ABA 문제란

ABA 문제는 CAS(Compare-And-Swap) 같은 원자적 연산을 사용하는 락프리(lock-free) 알고리즘에서 발생하는 동기화 문제

```
ptr = malloc(10);
free(ptr);
ptr = malloc(10);
```

같이 빠르게 할당 해제 할당하면 같은 주소를 부여함

여러드 쓰레드에서 큐에 접근시
T1: 테일 확인
T2: 테일 할당 해제
T3: 다른데이터 할당(주소 재사용) -> 헤드거나 헤드 근처

이후 T1 쓰레드는 헤드가 아니고 새로 추가된 노드에 접근하게 되어 데이터 일관성 깨짐

# 해결 방법

- Tagged Pointer (버전 번호 붙이기) → 많이 쓰는 방법

- Hazard Pointer: 다른 스레드가 해제한 노드를 아직 쓰고 있음을 명시

- RCU (Read-Copy-Update): 해제를 지연시킴

- Epoch-based Reclamation: 전체 스레드가 접근 안 한다고 확인되면 해제

### Tagged Pointer (버전 번호 붙이기)

할당받은 64비트 주소중 하위 3-4비트는 0으로 되어있음
아키텍쳐에따라 주소는 8이나 16으로 나눠떨어짐

따라서 하위 비트를 버전 sequence로 사용 가능
주소 + 버전이 동시에 같아질 확률은 매우 적어 ABA 문제 방지

태그제거후 사용해야됨!

### Epoch-based Reclamation

- 각쓰레드가 읽고있는 epoch기록후

- 모든 쓰레드들이 삭제할려는 epoch 이상이면 삭제

- 삭제 후보 리스트에 넣고 나중에 한번에 삭제 





# ABA 문제 발생 예시

```
Node* pop() {
    Node* old_top;
    Node* next;
    do {
        old_top = top;
        if (old_top == NULL) return NULL;
        next = old_top->next;
        usleep(1000); // T1이 멈춰 있는 사이 다른 스레드가 작업함
    } while (!__sync_bool_compare_and_swap(&top, old_top, next));
    return old_top;
}
```

락프리 스택에서 POP을 하는경우 다음 헤드를 확인하고 교체하는 사이에
다른 쓰레드에서 POP 후 이전 헤드 노드의 주소를 재사용하여 새로 PUSH했을때
주소만으로 같은 구조체인지 판단할수 없음