// 연속 호출 간격(back-to-back delta) 분포 + clock_getres 해상도
// 빌드: gcc -O2 -o latency latency.c
// 실행: ./latency [samples]   (기본 1000만)
#define _GNU_SOURCE
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <x86intrin.h>

static inline uint64_t ns_of(clockid_t id)
{
    struct timespec ts;
    clock_gettime(id, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + ts.tv_nsec;
}

// 메서드별로 delta 수집 루프를 인라인 생성. is_tick=1이면 TSC 틱 단위
#define DEFINE_COLLECT(name, expr)                       \
    static void collect_##name(uint64_t *d, long n)      \
    {                                                    \
        unsigned aux;                                    \
        (void)aux;                                       \
        uint64_t prev = (expr);                          \
        for (long i = 0; i < n; i++) {                   \
            uint64_t cur = (expr);                       \
            d[i] = cur - prev;                           \
            prev = cur;                                  \
        }                                                \
    }

DEFINE_COLLECT(rdtsc, __rdtsc())
DEFINE_COLLECT(lfence_rdtsc, (_mm_lfence(), __rdtsc()))
DEFINE_COLLECT(rdtscp, __rdtscp(&aux))
DEFINE_COLLECT(mono, ns_of(CLOCK_MONOTONIC))
DEFINE_COLLECT(mono_raw, ns_of(CLOCK_MONOTONIC_RAW))
DEFINE_COLLECT(realtime, ns_of(CLOCK_REALTIME))
DEFINE_COLLECT(mono_coarse, ns_of(CLOCK_MONOTONIC_COARSE))

struct method {
    const char *name;
    void (*collect)(uint64_t *, long);
    int is_tick;
    clockid_t clk;
};

static const struct method methods[] = {
    {"rdtsc", collect_rdtsc, 1, -1},
    {"lfence+rdtsc", collect_lfence_rdtsc, 1, -1},
    {"rdtscp", collect_rdtscp, 1, -1},
    {"CLOCK_MONOTONIC", collect_mono, 0, CLOCK_MONOTONIC},
    {"CLOCK_MONOTONIC_RAW", collect_mono_raw, 0, CLOCK_MONOTONIC_RAW},
    {"CLOCK_REALTIME", collect_realtime, 0, CLOCK_REALTIME},
    {"CLOCK_MONOTONIC_COARSE", collect_mono_coarse, 0, CLOCK_MONOTONIC_COARSE},
};

static int cmp_u64(const void *a, const void *b)
{
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}

static uint64_t pct(const uint64_t *s, long n, double p)
{
    long i = (long)(p * (n - 1));
    return s[i];
}

static double calibrate_tsc_ghz(void)
{
    uint64_t c0 = __rdtsc(), t0 = ns_of(CLOCK_MONOTONIC_RAW);
    struct timespec req = {0, 200000000};
    nanosleep(&req, NULL);
    uint64_t c1 = __rdtsc(), t1 = ns_of(CLOCK_MONOTONIC_RAW);
    return (double)(c1 - c0) / (double)(t1 - t0);
}

int main(int argc, char **argv)
{
    long n = argc > 1 ? atol(argv[1]) : 10000000;

    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    if (sched_setaffinity(0, sizeof(set), &set) != 0)
        perror("sched_setaffinity");

    double ghz = calibrate_tsc_ghz();
    printf("TSC %.3f GHz | samples=%ld | CPU0 고정\n", ghz, n);

    printf("\n[clock_getres]\n");
    for (size_t m = 0; m < sizeof(methods) / sizeof(methods[0]); m++) {
        if (methods[m].is_tick)
            continue;
        struct timespec r;
        clock_getres(methods[m].clk, &r);
        printf("  %-24s %ld.%09ld s  (%ld ns)\n", methods[m].name,
               (long)r.tv_sec, r.tv_nsec, (long)(r.tv_sec * 1000000000L + r.tv_nsec));
    }

    uint64_t *d = malloc(sizeof(uint64_t) * n);
    if (!d) {
        perror("malloc");
        return 1;
    }

    memset(d, 1, sizeof(uint64_t) * n); // page fault가 측정에 섞이지 않도록 미리 터치

    printf("\n[연속 호출 간격 분포]  단위: ns (rdtsc 계열은 틱→ns 환산, 괄호는 틱)\n");
    printf("%-24s %7s %7s %7s %7s %8s %9s %10s %12s %7s\n",
           "method", "min", "p50", "p90", "p99", "p99.9", "p99.99", "p99.999", "max", "zero%");

    for (size_t m = 0; m < sizeof(methods) / sizeof(methods[0]); m++) {
        const struct method *me = &methods[m];
        me->collect(d, n < 100000 ? n : 100000); // 워밍업
        me->collect(d, n);

        long zeros = 0;
        for (long i = 0; i < n; i++)
            zeros += d[i] == 0;
        qsort(d, n, sizeof(uint64_t), cmp_u64);

        double k = me->is_tick ? 1.0 / ghz : 1.0;
        printf("%-24s %7.1f %7.1f %7.1f %7.1f %8.1f %9.1f %10.1f %12.1f %6.2f%%\n",
               me->name, d[0] * k, pct(d, n, .5) * k, pct(d, n, .9) * k, pct(d, n, .99) * k,
               pct(d, n, .999) * k, pct(d, n, .9999) * k, pct(d, n, .99999) * k,
               d[n - 1] * k, 100.0 * zeros / n);
        if (me->is_tick)
            printf("%-24s %7lu %7lu %7lu %7lu %8lu %9lu %10lu %12lu  (ticks)\n", "",
                   d[0], pct(d, n, .5), pct(d, n, .9), pct(d, n, .99), pct(d, n, .999),
                   pct(d, n, .9999), pct(d, n, .99999), d[n - 1]);
    }

    free(d);
    return 0;
}
