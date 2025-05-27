# pthread_set_specific

```
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct Node {
    void *data;          // 동적 할당된 실제 데이터
    struct Node *next;   // 다음 노드
} Node;

pthread_key_t key;

// 🔁 destructor: 스레드 종료 시 연결 리스트 및 내부 데이터 해제
void destructor(void *ptr) {
    Node *current = (Node *)ptr;
    Node *temp;

    printf("[Destructor] 연결 리스트 해제 시작\n");

    while (current) {
        temp = current;
        printf(" > 데이터 해제: \"%s\"\n", (char *)current->data);
        free(current->data);  // 내부 동적 데이터 해제
        current = current->next;
        free(temp);           // 노드 자체 해제
    }
}

// 🧵 스레드 함수: 문자열 데이터 할당하여 연결 리스트에 저장
void *thread_func(void *arg) {
    long tid = (long)arg;
    Node *head = NULL;

    // 연결 리스트 구성 (예: 3개 노드)
    for (int i = 0; i < 3; i++) {
        Node *node = malloc(sizeof(Node));

        // 동적으로 문자열 데이터 생성
        char *str = malloc(64);
        snprintf(str, 64, "Thread %ld - Data %d", tid, i);

        node->data = str;
        node->next = head;
        head = node;
    }

    // 현재 스레드에 연결 리스트 저장
    pthread_setspecific(key, head);

    // (옵션) 리스트 출력
    Node *curr = head;
    while (curr) {
        printf("[Thread %ld] 노드 데이터: %s\n", tid, (char *)curr->data);
        curr = curr->next;
    }

    sleep(1);  // 작업 시뮬레이션
    return NULL;
}

int main() {
    pthread_t t1, t2;

    // 스레드별 저장소 key 생성
    pthread_key_create(&key, destructor);

    pthread_create(&t1, NULL, thread_func, (void *)1);
    pthread_create(&t2, NULL, thread_func, (void *)2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    pthread_key_delete(key);  // key 해제

    return 0;
}
```

POSIX 스레드(pthreads) 라이브러리에서 사용하는 함수로, 스레드 고유 데이터(thread-specific data, TLS)를 저장하는 데 사용됩니다. 이 함수는 각 스레드가 자신만의 데이터를 가질 수 있도록 해줍니다.

저장된 값은 오직 해당 스레드만 접근할 수 있습니다.
pthread_getspecific으로 나중에 값을 꺼낼 수 있습니다.
스레드 종료 시 자동으로 호출되는 소멸자(destructor) 를 지정할 수 있습니다.


✅ pthread_setspecific / TSD의 장점 정리
장점	설명
✅ 함수 인자로 전달할 필요 없음	전역처럼 접근 가능하지만 스레드마다 독립적인 값 유지.
✅ 로컬 캐싱 효과	스레드 고유 데이터로, 공유 자원 접근보다 빠르고 락이 필요 없음.
✅ 동적 할당 해제 가능	pthread_key_create 시 destructor 함수 지정 가능 — 스레드 종료 시 자동 해제됨.
✅ 캡슐화 가능	라이브러리 내부적으로 사용하면 외부에 인터페이스 노출 없이 TSD 유지 가능.
✅ 리엔트런트 코드 작성에 유리	전역 변수를 사용하지 않고 스레드마다 독립적인 상태를 유지할 수 있음.
✅ 라이브러리에서 상태 보존	예: errno, strtok, rand 등 전역 상태 유지가 필요한 함수들이 TSD를 활용해 재진입 가능하게 구현됨.
✅ 스레드 수에 따라 유연한 메모리 사용	고정 배열보다 효율적, 스레드별 필요시만 메모리 할당.

