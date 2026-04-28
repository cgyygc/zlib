/* benchmark_crc32.c -- Performance benchmark for CRC32 implementations
 * Copyright (C) 2024
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#define _POSIX_C_SOURCE 199309L  /* For clock_gettime */
#include "../../zlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Return time in seconds with high resolution */
static double get_time(void) {
#ifdef _WIN32
    /* Windows fallback */
    static LARGE_INTEGER frequency;
    LARGE_INTEGER t;
    if (frequency.QuadPart == 0) {
        QueryPerformanceFrequency(&frequency);
    }
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart / frequency.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
#endif
}

/* Fill buffer with a pattern */
static void fill_buffer(unsigned char *buf, size_t size, int pattern) {
    for (size_t i = 0; i < size; i++) {
        buf[i] = (unsigned char)((i + pattern) & 0xff);
    }
}

/* Benchmark a specific buffer size */
static void benchmark_size(const char *label, size_t size, int iterations) {
    unsigned char *buf = malloc(size);
    if (!buf) {
        printf("ERROR: Failed to allocate %zu bytes\n", size);
        return;
    }

    fill_buffer(buf, size, 0);

    /* Warm up - run once to ensure any initialization is done */
    unsigned long crc = crc32(0L, buf, (uInt)size);

    /* Measure */
    double start = get_time();
    for (int i = 0; i < iterations; i++) {
        crc = crc32(0L, buf, (uInt)size);
    }
    double end = get_time();

    double elapsed = end - start;
    double bytes_per_sec = (size * (double)iterations) / elapsed;
    double mb_per_sec = bytes_per_sec / (1024.0 * 1024.0);
    double ns_per_byte = (elapsed * 1e9) / (size * iterations);

    printf("%-20s %10zu bytes: %8.2f MB/s  (%.2f ns/byte)  [CRC: 0x%08lx]\n",
           label, size, mb_per_sec, ns_per_byte, crc);

    free(buf);
}

/* Print header */
static void print_header(int iterations) {
    printf("=========================================\n");
    printf("  CRC32 Performance Benchmark\n");
    printf("=========================================\n");
    printf("Iterations per test: %d\n", iterations);
    printf("\n");
#ifdef HAVE_RISCV_RVV
    printf("RVV support: ENABLED\n");
#else
    printf("RVV support: DISABLED (software only)\n");
#endif
    printf("\n");
}

/* Print footer */
static void print_footer(void) {
    printf("\n");
    printf("Notes:\n");
    printf("  - Results depend on CPU, cache size, and compiler\n");
    printf("  - First run may be slower due to cache warmup\n");
    printf("  - RVV threshold is 64 bytes\n");
    printf("\n");
}

int main(int argc, char *argv[]) {
    int iterations = 1000;

    /* Parse command line */
    if (argc > 1) {
        iterations = atoi(argv[1]);
        if (iterations < 1) iterations = 1000;
    }

    print_header(iterations);

    printf("%-20s %13s %18s %s\n", "Test", "Size", "Throughput", "Latency");
    printf("%-20s %13s %18s %s\n", "----", "----", "----------", "--------");

    /* Test sub-threshold sizes (software path) */
    printf("\n--- Below RVV threshold (< 64 bytes) ---\n");
    benchmark_size("1 byte", 1, iterations * 10000);
    benchmark_size("8 bytes", 8, iterations * 10000);
    benchmark_size("16 bytes", 16, iterations * 5000);
    benchmark_size("32 bytes", 32, iterations * 2000);
    benchmark_size("63 bytes", 63, iterations * 1000);

    /* Test at threshold (switching point) */
    printf("\n--- At RVV threshold (64 bytes) ---\n");
    benchmark_size("64 bytes", 64, iterations * 1000);

    /* Test above threshold (vectorized path) */
    printf("\n--- Above RVV threshold (> 64 bytes) ---\n");
    benchmark_size("128 bytes", 128, iterations * 500);
    benchmark_size("256 bytes", 256, iterations * 200);
    benchmark_size("512 bytes", 512, iterations * 100);
    benchmark_size("1 KB", 1024, iterations * 100);
    benchmark_size("2 KB", 2048, iterations * 50);
    benchmark_size("4 KB", 4096, iterations * 50);
    benchmark_size("8 KB", 8192, iterations * 20);
    benchmark_size("16 KB", 16384, iterations * 10);
    benchmark_size("32 KB", 32768, iterations * 10);
    benchmark_size("64 KB", 65536, iterations * 5);
    benchmark_size("128 KB", 131072, iterations * 2);
    benchmark_size("256 KB", 262144, iterations);
    benchmark_size("512 KB", 524288, iterations / 2);
    benchmark_size("1 MB", 1048576, iterations / 2);
    benchmark_size("2 MB", 2097152, iterations / 4);
    benchmark_size("4 MB", 4194304, iterations / 8);

    /* Large file sizes */
    printf("\n--- Large file sizes ---\n");
    benchmark_size("8 MB", 8388608, iterations / 10);
    benchmark_size("16 MB", 16777216, iterations / 20);
    benchmark_size("32 MB", 33554432, iterations / 40);

    print_footer();

    return 0;
}
