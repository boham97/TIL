// rdtsc vs clock_gettime 호출 비용 벤치마크
// 빌드: gcc -O2 -o bench bench.c -lm
// 실행: ./bench [trials]   (기본 20회)
#define _GNU_SOURCE
#include <math.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <x86intrin.h>

static volatile uint64_t sink;

static inline uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + ts.tv_nsec;
}

static inline uint64_t ts_sum(clockid_t id)
{
    struct timespec ts;
    clock_gettime(id, &ts);
    return ts.tv_sec + ts.tv_nsec;
}

// 메서드별로 루프를 인라인 생성 (함수 포인터 간접 호출 오버헤드 배제)
#define DEFINE_LOOP(name, expr)                    \
    static uint64_t loop_##name(long n)            \
    {                                              \
        uint64_t acc = 0;                          \
        unsigned aux;                              \
        (void)aux;                                 \
        uint64_t t0 = now_ns();                    \
        for (long i = 0; i < n; i++)               \
            acc += (expr);                         \
        uint64_t t1 = now_ns();                    \
        sink = acc;                                \
        return t1 - t0;                            \
    }

DEFINE_LOOP(rdtsc, __rdtsc())
DEFINE_LOOP(lfence_rdtsc, (_mm_lfence(), __rdtsc()))
DEFINE_LOOP(rdtscp, __rdtscp(&aux))
DEFINE_LOOP(mono, ts_sum(CLOCK_MONOTONIC))
DEFINE_LOOP(mono_raw, ts_sum(CLOCK_MONOTONIC_RAW))
DEFINE_LOOP(realtime, ts_sum(CLOCK_REALTIME))
DEFINE_LOOP(mono_coarse, ts_sum(CLOCK_MONOTONIC_COARSE))

struct method {
    const char *name;
    uint64_t (*loop)(long);
};

static const struct method methods[] = {
    {"rdtsc", loop_rdtsc},
    {"lfence+rdtsc", loop_lfence_rdtsc},
    {"rdtscp", loop_rdtscp},
    {"CLOCK_MONOTONIC", loop_mono},
    {"CLOCK_MONOTONIC_RAW", loop_mono_raw},
    {"CLOCK_REALTIME", loop_realtime},
    {"CLOCK_MONOTONIC_COARSE", loop_mono_coarse},
};

static const long counts[] = {1000, 10000, 100000, 1000000, 10000000};

static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static double percentile(const double *sorted, int n, double p)
{
    double idx = p * (n - 1);
    int lo = (int)idx;
    int hi = lo + 1 < n ? lo + 1 : lo;
    return sorted[lo] + (sorted[hi] - sorted[lo]) * (idx - lo);
}

static double calibrate_tsc_ghz(void)
{
    uint64_t c0 = __rdtsc(), t0 = now_ns();
    struct timespec req = {0, 200000000};
    nanosleep(&req, NULL);
    uint64_t c1 = __rdtsc(), t1 = now_ns();
    return (double)(c1 - c0) / (double)(t1 - t0);
}

int main(int argc, char **argv)
{
    int trials = argc > 1 ? atoi(argv[1]) : 20;
    if (trials < 2)
        trials = 2;

    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    if (sched_setaffinity(0, sizeof(set), &set) != 0)
        perror("sched_setaffinity");

    double ghz = calibrate_tsc_ghz();
    printf("TSC 주파수(측정): %.3f GHz  |  trials=%d  |  CPU0 고정\n", ghz, trials);
    printf("단위: ns/call (각 trial = N회 호출 총시간 / N)\n");

    double *samples = malloc(sizeof(double) * trials);

    for (size_t m = 0; m < sizeof(methods) / sizeof(methods[0]); m++) {
        const struct method *me = &methods[m];
        me->loop(100000); // 워밍업

        printf("\n== %s ==\n", me->name);
        printf("%10s %8s %8s %8s %8s %8s %8s %7s %8s\n",
               "N", "mean", "stddev", "min", "median", "p95", "max", "CV%", "cycles");

        for (size_t c = 0; c < sizeof(counts) / sizeof(counts[0]); c++) {
            long n = counts[c];
            double sum = 0, sq = 0;
            for (int t = 0; t < trials; t++) {
                samples[t] = (double)me->loop(n) / n;
                sum += samples[t];
            }
            double mean = sum / trials;
            for (int t = 0; t < trials; t++)
                sq += (samples[t] - mean) * (samples[t] - mean);
            double sd = sqrt(sq / (trials - 1));
            qsort(samples, trials, sizeof(double), cmp_double);

            printf("%10ld %8.2f %8.2f %8.2f %8.2f %8.2f %8.2f %7.1f %8.1f\n",
                   n, mean, sd, samples[0], percentile(samples, trials, 0.5),
                   percentile(samples, trials, 0.95), samples[trials - 1],
                   100.0 * sd / mean, percentile(samples, trials, 0.5) * ghz);
        }
    }

    free(samples);
    return 0;
}
