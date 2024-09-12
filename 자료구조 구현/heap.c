#include<stdio.h>
#include<stdlib.h>



typedef struct{
    long msg_id;
    char content[30];
} Message;
struct Node
{
    long data;
};

struct Heap
{
    struct Node* arr[MAX];
    int cnt;
};

struct Heap* heap_init(){
    struct Heap* heap = malloc(sizeof(struct Heap));
    heap->cnt = 1;
    for (int i = 0; i< MAX; i++){
        heap->arr[i] == NULL;
    }
    return heap;
}
void node_free_post(struct Heap* heap, int index){
    if(heap->arr[index] == NULL) return;
    node_free_post(heap, index * 2);
    node_free_post(heap, index * 2 + 1);
    free(heap->arr[index]);
}
void heap_free(struct Heap* heap){
    node_free_post(heap, 1);
    free(heap);
}
void heappush(struct Heap* heap, long data){
    int parent;
    int target = heap->cnt++;
    struct Node* parent_node;
    struct Node* son_node;
    struct Node* temp;
    struct Node* node = (struct Node*)malloc(sizeof(struct Node));
    node->data = data;
    heap->arr[target] = node;
    while(target != 1){
        parent = target/2;
        parent_node = heap->arr[parent];
        son_node = heap->arr[target];
        if(parent_node->data < son_node->data) break;
        temp = parent_node;
        heap->arr[parent] = son_node;
        heap->arr[target] = temp;
        target /= 2;
    }
}
long heappop(struct Heap* heap){
    if (heap->arr[1] == NULL) return -1l;
    long res = heap->arr[1]->data;
    int index = 1;
    int left;
    int right;
    int target;
    struct Node* parent_node;
    struct Node* son_node;
    struct Node* temp;
    free(heap->arr[1]);
    heap->cnt--;
    heap->arr[1] = heap->arr[heap->cnt];
    heap->arr[heap->cnt] = NULL;

    while (index < heap->cnt){
        left = __INT_MAX__;
        right = __INT_MAX__;
        target = 0;
        if (2 * index < MAX && heap->arr[2 * index] != NULL && heap->arr[index]->data > heap->arr[2 * index]->data) left = heap->arr[2 * index]->data;
        if (2 * index + 1< MAX && heap->arr[2 * index + 1] != NULL && heap->arr[index]->data > heap->arr[2 * index + 1]->data) right = heap->arr[2 * index + 1]->data;
        if (left < right){
            target = 2 * index;
        }else if (left > right){
            target = 2 * index + 1;
        }else{
            break;
        }
        parent_node = heap->arr[index];
        son_node = heap->arr[target];
        temp = parent_node;
        heap->arr[index] = son_node;
        heap->arr[target] = temp;
        index = target;
    }
    return res;
}
int heappeak(struct Heap* heap){return heap->arr[1]->data;}
int heapcnt(struct Heap* heap){return heap->cnt - 1;}
int main(int argc, char *argv[]){
    return 0;
}
