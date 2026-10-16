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

// 2) 분기 예측 실패 → branchless로 바꾼 버전 (cmov/벡터화 금지 속성은 그대로 둠)
__attribute__((optimize("no-if-conversion","no-if-conversion2","no-tree-vectorize")))
long branchy(const int *v) {
    long s = 0;
    for (int r = 0; r < 10; r++)
        for (int i = 0; i < M; i++)
            s += (v[i] < 128) * v[i];   // branchless: 비교 결과(0/1)를 곱함
    return s;
}

// 3) 순수 연산 부하 (compute만 -O3 -ffast-math, 카운터를 int로: AVX2 vcvtdq2pd로 SIMD 변환 가능)
__attribute__((optimize("O3","fast-math")))
double compute(void) {
    double x = 0;
    for (int i = 1; i < 200000000; i++)
        x += 1.0 / i;
    return x;
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
