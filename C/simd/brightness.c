#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <immintrin.h>  // AVX2

#define WIDTH  4096
#define HEIGHT 4096
#define N      (WIDTH * HEIGHT)  // 16,777,216 픽셀

// 스칼라 버전: 픽셀 하나씩 처리
void brightness_scalar(const uint8_t *src, uint8_t *dst, int n, uint8_t add) {
    for (int i = 0; i < n; i++) {
        int v = src[i] + add;
        dst[i] = v > 255 ? 255 : (uint8_t)v;  // clamp
    }
}

// AVX2 버전: 한 번에 32픽셀 처리
void brightness_avx2(const uint8_t *src, uint8_t *dst, int n, uint8_t add) {
    __m256i vadds = _mm256_set1_epi8((int8_t)add);  // 32개 레인에 add 값 복제

    int i = 0;
    for (; i <= n - 32; i += 32) {
        __m256i chunk = _mm256_loadu_si256((__m256i *)(src + i));
        // _mm256_adds_epu8: unsigned saturating add (255 초과 시 자동 clamp)
        __m256i result = _mm256_adds_epu8(chunk, vadds);
        _mm256_storeu_si256((__m256i *)(dst + i), result);
    }
    // 나머지 픽셀 (32의 배수가 아닌 끝부분)
    for (; i < n; i++) {
        int v = src[i] + add;
        dst[i] = v > 255 ? 255 : (uint8_t)v;
    }
}

static double now_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

int main() {
    uint8_t *src   = aligned_alloc(32, N);
    uint8_t *dst_s = aligned_alloc(32, N);
    uint8_t *dst_v = aligned_alloc(32, N);

    // 랜덤 픽셀 데이터 생성
    srand(42);
    for (int i = 0; i < N; i++) src[i] = rand() & 0xFF;

    const uint8_t BRIGHTNESS = 50;
    const int REPEAT = 10;

    // ---------- 스칼라 ----------
    double t0 = now_ms();
    for (int r = 0; r < REPEAT; r++)
        brightness_scalar(src, dst_s, N, BRIGHTNESS);
    double scalar_ms = (now_ms() - t0) / REPEAT;

    // ---------- AVX2 ----------
    double t1 = now_ms();
    for (int r = 0; r < REPEAT; r++)
        brightness_avx2(src, dst_v, N, BRIGHTNESS);
    double avx2_ms = (now_ms() - t1) / REPEAT;

    // 결과 검증
    int ok = memcmp(dst_s, dst_v, N) == 0;

    printf("이미지 크기: %dx%d = %d 픽셀\n\n", WIDTH, HEIGHT, N);
    printf("스칼라  : %.2f ms\n", scalar_ms);
    printf("AVX2    : %.2f ms\n", avx2_ms);
    printf("속도 향상: %.1fx\n\n", scalar_ms / avx2_ms);
    printf("결과 검증: %s\n", ok ? "PASS (두 결과 동일)" : "FAIL");

    free(src); free(dst_s); free(dst_v);
    return 0;
}
