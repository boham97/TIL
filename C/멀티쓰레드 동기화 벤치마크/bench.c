#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>

#define BUFFER_SIZE      64
#define NUM_PRODUCERS    8
#define NUM_CONSUMERS    8
#define ITEMS_PER_PRODUCER 1000000
#define POOL_SIZE        (NUM_PRODUCERS * ITEMS_PER_PRODUCER + 16)

/* ──────────────────────────────────────────
 * 뮤텍스 기반 생산자-소비자
 * ────────────────────────────────────────── */
typedef struct {
    int buf[BUFFER_SIZE];
    int head, tail, count;
    pthread_mutex_t lock;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
    int done;
} MutexQueue;

MutexQueue mq;

void mq_init(MutexQueue *q) {
    q->head = q->tail = q->count = q->done = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_full, NULL);
    pthread_cond_init(&q->not_empty, NULL);
}

void *mutex_producer(void *arg) {
    int id = *(int*)arg;
    for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
        int item = id * 1000 + i;
        pthread_mutex_lock(&mq.lock);
        while (mq.count == BUFFER_SIZE)
            pthread_cond_wait(&mq.not_full, &mq.lock);
        mq.buf[mq.tail] = item;
        mq.tail = (mq.tail + 1) % BUFFER_SIZE;
        mq.count++;
        pthread_cond_signal(&mq.not_empty);
        pthread_mutex_unlock(&mq.lock);
    }
    return NULL;
}

void *mutex_consumer(void *arg) {
    (void)arg;
    long consumed = 0;
    while (1) {
        pthread_mutex_lock(&mq.lock);
        while (mq.count == 0 && !mq.done)
            pthread_cond_wait(&mq.not_empty, &mq.lock);
        if (mq.count == 0 && mq.done) {
            pthread_mutex_unlock(&mq.lock);
            break;
        }
        mq.head = (mq.head + 1) % BUFFER_SIZE;
        mq.count--;
        pthread_cond_signal(&mq.not_full);
        pthread_mutex_unlock(&mq.lock);
        consumed++;
    }
    return (void*)consumed;
}

/* ──────────────────────────────────────────
 * CAS 기반 lock-free 스택 (노드 풀)
 * ────────────────────────────────────────── */
typedef struct Node {
    int val;
    struct Node *next;
} Node;

typedef struct {
    _Atomic(Node*) top;
    atomic_long push_count;
    atomic_long pop_count;
} CASStack;

CASStack cs;
atomic_int cas_done;

/* 노드 풀 - malloc/free 제거로 ABA/use-after-free 방지 */
static Node node_pool[POOL_SIZE];
static atomic_int pool_idx;

Node *pool_alloc(void) {
    int idx = atomic_fetch_add(&pool_idx, 1);
    return &node_pool[idx];
}

void cas_push(CASStack *s, int val) {
    Node *node = pool_alloc();
    node->val = val;
    Node *old;
    do {
        old = atomic_load(&s->top);
        node->next = old;
    } while (!atomic_compare_exchange_weak(&s->top, &old, node));
    atomic_fetch_add(&s->push_count, 1);
}

int cas_pop(CASStack *s, int *out) {
    Node *old, *next;
    do {
        old = atomic_load(&s->top);
        if (!old) return 0;
        next = old->next;
    } while (!atomic_compare_exchange_weak(&s->top, &old, next));
    *out = old->val;
    atomic_fetch_add(&s->pop_count, 1);
    return 1;
}

void *cas_producer(void *arg) {
    int id = *(int*)arg;
    for (int i = 0; i < ITEMS_PER_PRODUCER; i++)
        cas_push(&cs, id * 1000 + i);
    return NULL;
}

void *cas_consumer(void *arg) {
    (void)arg;
    long consumed = 0;
    int val;
    while (1) {
        if (cas_pop(&cs, &val)) {
            consumed++;
        } else if (atomic_load(&cas_done)) {
            break;
        }
    }
    return (void*)consumed;
}

/* ──────────────────────────────────────────
 * 시간 측정
 * ────────────────────────────────────────── */
double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

/* ──────────────────────────────────────────
 * 실행
 * ────────────────────────────────────────── */
void run_mutex(void) {
    mq_init(&mq);

    pthread_t prod[NUM_PRODUCERS], cons[NUM_CONSUMERS];
    int ids[NUM_PRODUCERS];

    double t0 = now_ms();

    for (int i = 0; i < NUM_PRODUCERS; i++) {
        ids[i] = i;
        pthread_create(&prod[i], NULL, mutex_producer, &ids[i]);
    }
    for (int i = 0; i < NUM_CONSUMERS; i++)
        pthread_create(&cons[i], NULL, mutex_consumer, NULL);

    for (int i = 0; i < NUM_PRODUCERS; i++)
        pthread_join(prod[i], NULL);

    pthread_mutex_lock(&mq.lock);
    mq.done = 1;
    pthread_cond_broadcast(&mq.not_empty);
    pthread_mutex_unlock(&mq.lock);

    long total = 0;
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        void *ret;
        pthread_join(cons[i], &ret);
        total += (long)ret;
    }

    printf("[MUTEX]  소비 합계: %ld  시간: %.2f ms\n", total, now_ms() - t0);
}

void run_cas(void) {
    atomic_store(&cs.top, NULL);
    atomic_store(&cs.push_count, 0);
    atomic_store(&cs.pop_count, 0);
    atomic_store(&cas_done, 0);
    atomic_store(&pool_idx, 0);

    pthread_t prod[NUM_PRODUCERS], cons[NUM_CONSUMERS];
    int ids[NUM_PRODUCERS];

    double t0 = now_ms();

    for (int i = 0; i < NUM_PRODUCERS; i++) {
        ids[i] = i;
        pthread_create(&prod[i], NULL, cas_producer, &ids[i]);
    }
    for (int i = 0; i < NUM_CONSUMERS; i++)
        pthread_create(&cons[i], NULL, cas_consumer, NULL);

    for (int i = 0; i < NUM_PRODUCERS; i++)
        pthread_join(prod[i], NULL);

    atomic_store(&cas_done, 1);

    long total = 0;
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        void *ret;
        pthread_join(cons[i], &ret);
        total += (long)ret;
    }

    printf("[CAS]    소비 합계: %ld  시간: %.2f ms\n", total, now_ms() - t0);
    printf("         push: %ld  pop: %ld\n",
           atomic_load(&cs.push_count),
           atomic_load(&cs.pop_count));
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("사용법: %s [mutex|cas|all]\n", argv[0]);
        return 1;
    }

    int do_mutex = strcmp(argv[1], "mutex") == 0 || strcmp(argv[1], "all") == 0;
    int do_cas   = strcmp(argv[1], "cas")   == 0 || strcmp(argv[1], "all") == 0;

    if (!do_mutex && !do_cas) {
        printf("알 수 없는 인자: %s\n", argv[1]);
        return 1;
    }

    printf("생산자 %d개 / 소비자 %d개 / 생산량 %d개씩\n\n",
           NUM_PRODUCERS, NUM_CONSUMERS, ITEMS_PER_PRODUCER);

    if (do_mutex) run_mutex();
    if (do_cas)   run_cas();

    return 0;
}
