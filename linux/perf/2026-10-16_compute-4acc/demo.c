// perf 실습용 예제: 일부러 느린 패턴 3가지
#include <stdio.h>
#include <stdlib.h>

#define N 2048
#define M (1 << 24)

static double a[N][N];

// 1) 캐시 미스: 열 우선(column-major) 순회
double sum_cols(void) {
    double s = 0;
    for (int j = 0; j < N; j++)
        for (int i = 0; i < N; i++)
            s += a[i][j];
    return s;
}

// 비교용: 행 우선 순회 (캐시 친화적)
double sum_rows(void) {
    double s = 0;
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            s += a[i][j];
    return s;
}

// 2) 분기 예측 실패: 랜덤 데이터에 대한 조건 분기
__attribute__((optimize("no-if-conversion","no-if-conversion2","no-tree-vectorize")))
long branchy(const int *v) {
    long s = 0;
    for (int r = 0; r < 10; r++)
        for (int i = 0; i < M; i++)
            if (v[i] < 128)
                s += v[i];
    return s;
}

// 3) 순수 연산 부하
// 누산기 4개로 나눈 버전 (덧셈 의존 사슬을 4갈래로 분리)
double compute(void) {
    const long L = 200000000;
    double a = 0, b = 0, c = 0, d = 0;
    long i;
    for (i = 1; i + 3 < L; i += 4) {
        a += 1.0 / i; b += 1.0 / (i + 1); c += 1.0 / (i + 2); d += 1.0 / (i + 3);
    }
    for (; i < L; i++)   // 나머지 (원본과 같은 범위 1..L-1)
        a += 1.0 / i;
    return a + b + c + d;
}

int main(void) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            a[i][j] = i + j;

    int *v = malloc(sizeof(int) * M);
    srand(42);
    for (int i = 0; i < M; i++)
        v[i] = rand() % 256;

    double r1 = 0, r2 = 0;
    for (int k = 0; k < 5; k++) {
        r1 += sum_cols();
        r2 += sum_rows();
    }
    long r3 = branchy(v);
    double r4 = compute();

    printf("%f %f %ld %f\n", r1, r2, r3, r4);
    free(v);
    return 0;
}
