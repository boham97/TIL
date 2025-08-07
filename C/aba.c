#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

typedef struct Node {
    int value;
    struct Node* next;
} Node;

Node* top = NULL;

typedef struct {
    Node* ptr;
    uint64_t tag;
} __attribute__((packed)) TaggedPtr;



void push(Node* node) {
    Node* old_top;
    do {
        old_top = top;
        node->next = old_top;
    } while (!__sync_bool_compare_and_swap(&top, old_top, node));
}

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

Node* pop2() {
    Node* old_top;
    Node* next;
    do {
        old_top = top;
        if (old_top == NULL) return NULL;
        next = old_top->next;

    } while (!__sync_bool_compare_and_swap(&top, old_top, next));
    return old_top;
}

Node* nodeA;
Node* nodeB;

void* thread1(void* arg) {
    // T1: pop 시도 (ABA 문제 피해자)
    printf("T1: popping...\n");
    Node* n = pop();
    printf("T1: popped node with value %d\n", n->value);
    return NULL;
}

void* thread2(void* arg) {
    // T2: pop -> free -> push (ABA 문제 유발자)
    usleep(500); // T1보다 약간 늦게 시작

    Node* n1 = pop2();      // pop A
    printf("T2: popped node with value %d\n", n1->value);

    Node* n2 = (Node*)malloc(sizeof(Node)); // 새 노드 B
    n2->value = 2;
    push(n2); // push B

    push(n1); // 다시 A 푸시 → top이 원래 A로 복구됨
    printf("T2: pushed back node with value %d (A)\n", n1->value);
    return NULL;
}

int main() {
    // 초기화
    nodeA = (Node*)malloc(sizeof(Node));
    nodeA->value = 1;
    nodeA->next = NULL;
    top = nodeA;

    pthread_t t1, t2;

    pthread_create(&t1, NULL, thread1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}
