#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

struct Node
{
    struct Node* next;
    int data;
};
struct Queue
{
    int node_cnt;
    struct Node* head;
    struct Node* tail;
};

int queue_len(struct Queue* que){
    return que->node_cnt;
}
struct Queue* queue_init(){
    struct Queue* new_queue = malloc(sizeof(struct Queue));  
    new_queue->head = NULL;
    new_queue->tail = NULL;
    new_queue->node_cnt = 0;
    return new_queue;
};
void append(struct Queue* que, int data){
    struct Node* new_node = malloc(sizeof(struct Node));
    new_node->data = data;
    if (que->head == NULL){
        que->head = new_node;
        que->tail = new_node;
        que->node_cnt = 1;
    }else{
        que->tail->next = new_node;
        que->tail = new_node;
        que->node_cnt++;
    }
}
int popleft(struct Queue* que){
    if (que->head == NULL)
        return -1;
    struct Node* poped_node = que->head;
    que->head = poped_node->next;
    que->node_cnt--;
    int res = poped_node->data;
    free(poped_node);
    return res;
}
int main(){
    int cmd;
    int value;
    int roop =1;
    struct Queue* que = queue_init();
    while(roop){
        printf("insert cmd: ");
        scanf("%d", &cmd);
        switch (cmd)
        {
        case 1:
            printf("enqueue: ");
            scanf("%d", &value);
            append(que, value);
            break;
        case 2:
            value = popleft(que);
            printf("deque: %d\n", value);
            break;
        default:
            roop = 0;
            break;
        }
    }
    return 0;
}