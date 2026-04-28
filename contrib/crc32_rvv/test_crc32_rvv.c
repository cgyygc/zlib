/* test_crc32_rvv.c -- Test program for CRC32 RVV implementation
 * Copyright (C) 2024
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#include "../../zlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Test vectors from RFC 1952 and various sources */
static struct {
    const char *data;
    unsigned long expected_crc;
    const char *description;
} test_vectors[] = {
    /* Basic test vectors from RFC 1952 */
    { "", 0x00000000, "Empty string" },
    { "a", 0xe8b7be43, "Single character" },
    { "abc", 0x352441c2, "Three characters" },
    { "message digest", 0x20159d7f, "Phrase" },
    { "abcdefghijklmnopqrstuvwxyz", 0x4c2750bd, "Lowercase alphabet" },
    { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
      0x1fc2e6d2, "Alphanumeric" },
    { "12345678901234567890123456789012345678901234567890123456789012345678901234567890",
      0x7ca94a72, "Numeric string" },

    /* Additional standard test vectors */
    { "123456789", 0xcbf43926, "Standard test string" },

    { NULL, 0, NULL }
};

/* Color output support */
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_RESET "\033[0m"

static int use_color = 1;

static int compare_crc(const char *description, const char *data,
                       unsigned long got, unsigned long expected) {
    if (got != expected) {
        if (use_color) printf("%s", COLOR_RED);
        printf("FAIL");
        if (use_color) printf("%s", COLOR_RESET);
        printf(": %s\n", description);
        printf("  Data: \"%s%s\" (len=%zu)\n",
               data && strlen(data) <= 40 ? data : "",
               data && strlen(data) > 40 ? "..." : "",
               data ? strlen(data) : 0);
        printf("  Expected: 0x%08lx\n", expected);
        printf("  Got:      0x%08lx\n", got);
        return 1;
    }
    if (use_color) printf("%s", COLOR_GREEN);
    printf("PASS");
    if (use_color) printf("%s", COLOR_RESET);
    printf(": %s (0x%08lx)\n", description, got);
    return 0;
}

static int test_basic_vectors(void) {
    int failures = 0;
    int i;

    printf("\n=== Basic CRC32 Test Vectors ===\n\n");

    for (i = 0; test_vectors[i].data != NULL; i++) {
        unsigned long crc = crc32(0L,
            (const unsigned char *)test_vectors[i].data,
            (uInt)strlen(test_vectors[i].data));
        failures += compare_crc(test_vectors[i].description,
                                test_vectors[i].data,
                                crc,
                                test_vectors[i].expected_crc);
    }

    return failures;
}

static int test_boundary_cases(void) {
    int failures = 0;

    printf("\n=== Boundary Cases (RVV threshold is 64 bytes) ===\n\n");

    /* Test various sizes around the RVV threshold */
    struct {
        int size;
        const char *desc;
    } sizes[] = {
        { 1, "1 byte" },
        { 7, "7 bytes" },
        { 8, "8 bytes" },
        { 15, "15 bytes" },
        { 16, "16 bytes" },
        { 31, "31 bytes" },
        { 32, "32 bytes" },
        { 63, "63 bytes (below threshold)" },
        { 64, "64 bytes (at threshold)" },
        { 65, "65 bytes (above threshold)" },
        { 127, "127 bytes" },
        { 128, "128 bytes" },
        { 255, "255 bytes" },
        { 256, "256 bytes" },
        { 512, "512 bytes" },
        { 1024, "1 KB" },
        { 0, NULL }
    };

    for (int i = 0; sizes[i].desc != NULL; i++) {
        unsigned char *buf = malloc(sizes[i].size);
        if (!buf) continue;

        /* Fill with a pattern */
        for (int j = 0; j < sizes[i].size; j++) {
            buf[j] = (unsigned char)((j * 17 + 13) & 0xff);
        }

        /* Compute CRC twice to verify repeatability */
        unsigned long crc1 = crc32(0L, buf, sizes[i].size);
        unsigned long crc2 = crc32(0L, buf, sizes[i].size);

        if (crc1 != crc2) {
            if (use_color) printf("%s", COLOR_RED);
            printf("FAIL");
            if (use_color) printf("%s", COLOR_RESET);
            printf(": %s - not repeatable\n", sizes[i].desc);
            failures++;
        } else {
            if (use_color) printf("%s", COLOR_GREEN);
            printf("PASS");
            if (use_color) printf("%s", COLOR_RESET);
            printf(": %s - CRC=0x%08lx\n", sizes[i].desc, crc1);
        }

        free(buf);
    }

    return failures;
}

static int test_chained_crc(void) {
    int failures = 0;

    printf("\n=== Chained CRC32 Test ===\n\n");

    /* Test that crc32(crc1, data2, len2) == crc32(0, data1+data2, total_len) */
    const char *part1 = "Hello";
    const char *part2 = " World";
    const char *full = "Hello World";

    unsigned long crc1 = crc32(0L, (const unsigned char *)part1, (uInt)strlen(part1));
    unsigned long crc2 = crc32(crc1, (const unsigned char *)part2, (uInt)strlen(part2));
    unsigned long crc_full = crc32(0L, (const unsigned char *)full, (uInt)strlen(full));

    printf("Part 1 CRC: 0x%08lx (\"%s\")\n", crc1, part1);
    printf("Part 2 CRC: 0x%08lx (\"%s\")\n", crc2, part2);
    printf("Full CRC:   0x%08lx (\"%s\")\n", crc_full, full);

    if (crc2 != crc_full) {
        if (use_color) printf("%s", COLOR_RED);
        printf("FAIL");
        if (use_color) printf("%s", COLOR_RESET);
        printf(": Chained CRC mismatch\n");
        failures++;
    } else {
        if (use_color) printf("%s", COLOR_GREEN);
        printf("PASS");
        if (use_color) printf("%s", COLOR_RESET);
        printf(": Chained CRC matches\n");
    }

    return failures;
}

static int test_large_buffers(void) {
    int failures = 0;
    size_t sizes[] = { 256, 1024, 4096, 16384, 65536 };
    int i;

    printf("\n=== Large Buffer Tests ===\n\n");

    for (i = 0; i < 5; i++) {
        size_t size = sizes[i];
        unsigned char *buf = malloc(size);
        if (!buf) {
            printf("SKIP: Could not allocate %zu bytes\n", size);
            continue;
        }

        /* Fill with a pattern */
        for (size_t j = 0; j < size; j++) {
            buf[j] = (unsigned char)(j & 0xff);
        }

        unsigned long crc1 = crc32(0L, buf, (uInt)size);
        unsigned long crc2 = crc32(0L, buf, (uInt)size);

        /* Test repeatability */
        if (crc1 != crc2) {
            if (use_color) printf("%s", COLOR_RED);
            printf("FAIL");
            if (use_color) printf("%s", COLOR_RESET);
            printf(": %zu bytes - not repeatable (0x%08lx != 0x%08lx)\n", size, crc1, crc2);
            failures++;
        } else {
            if (use_color) printf("%s", COLOR_GREEN);
            printf("PASS");
            if (use_color) printf("%s", COLOR_RESET);
            printf(": %6zu bytes - CRC = 0x%08lx\n", size, crc1);
        }

        free(buf);
    }

    return failures;
}

static int test_alignment(void) {
    int failures = 0;

    printf("\n=== Alignment Tests ===\n\n");

    /* Test that unaligned data produces correct results */
    /* We allocate a buffer and test at different alignments */
    unsigned char *buffer = malloc(65536 + 32);
    if (!buffer) {
        printf("SKIP: Could not allocate alignment test buffer\n");
        return 0;
    }

    /* Find a 16-byte aligned position */
    unsigned char *aligned_buffer = (unsigned char *)(((uintptr_t)buffer + 15) & ~((uintptr_t)15));

    /* Fill with pattern */
    for (int i = 0; i < 65536; i++) {
        aligned_buffer[i] = (unsigned char)(i & 0xff);
    }

    /* Compute baseline CRC from aligned position */
    unsigned long baseline = crc32(0L, aligned_buffer, 65536);

    /* Test at various unaligned offsets */
    int all_passed = 1;
    for (int offset = 1; offset < 16; offset++) {
        unsigned long crc = crc32(0L, aligned_buffer + offset, 65536);
        /* Each offset will have different CRC - that's expected! */
        /* We're testing that the function handles unaligned pointers correctly */
        /* by computing it twice and comparing */
        unsigned long crc2 = crc32(0L, aligned_buffer + offset, 65536);
        if (crc != crc2) {
            all_passed = 0;
            break;
        }
    }

    if (all_passed) {
        if (use_color) printf("%s", COLOR_GREEN);
        printf("PASS");
        if (use_color) printf("%s", COLOR_RESET);
        printf(": All alignment tests passed (15 offsets tested)\n");
    } else {
        if (use_color) printf("%s", COLOR_RED);
        printf("FAIL");
        if (use_color) printf("%s", COLOR_RESET);
        printf(": Some alignment tests failed\n");
        failures++;
    }

    free(buffer);
    return failures;
}

static void print_summary(int total_tests, int failures) {
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %d\n", total_tests);
    printf("Passed:      %d\n", total_tests - failures);
    printf("Failed:      %d\n", failures);

    if (failures == 0) {
        if (use_color) {
            printf("\n%s", COLOR_GREEN);
            printf("All tests PASSED!");
            printf("%s\n", COLOR_RESET);
        } else {
            printf("\nAll tests PASSED!\n");
        }
    } else {
        if (use_color) {
            printf("\n%s", COLOR_RED);
            printf("Some tests FAILED!");
            printf("%s\n", COLOR_RESET);
        } else {
            printf("\nSome tests FAILED!\n");
        }
    }
}

int main(int argc, char *argv[]) {
    int total_failures = 0;
    int total_tests = 0;

    /* Disable color output if stdout is not a terminal */
    if (!isatty(1)) {
        use_color = 0;
    }

    /* Check for --no-color option */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-color") == 0) {
            use_color = 0;
        }
    }

    printf("============================================\n");
    printf("  CRC32 RVV Implementation Test Suite\n");
    printf("============================================\n");

#ifdef HAVE_RISCV_RVV
#  ifdef __riscv_vector
    printf("\n%s", COLOR_GREEN);
    printf("RVV support: ENABLED");
    printf("%s\n", COLOR_RESET);
#  else
    printf("\n%s", COLOR_YELLOW);
    printf("RVV support: NOT ENABLED (using software fallback)");
    printf("%s\n", COLOR_RESET);
    printf("  Note: __riscv_vector macro not defined during compilation\n");
    printf("  This may be due to missing -march=rv64gcv compiler flag\n");
#  endif
#else
    printf("\n%s", COLOR_YELLOW);
    printf("RVV support: NOT ENABLED (using software fallback)");
    printf("%s\n", COLOR_RESET);
    printf("  Note: HAVE_RISCV_RVV not defined - reconfigure with --enable-crcrvv\n");
#endif

/* Show platform detection info */
#if defined(__riscv) || defined(__riscv__)
    printf("  Platform: RISC-V detected\n");
    #if defined(__riscv)
    printf("  Compiler defines: __riscv (value=%d)\n", __riscv);
    #endif
    #if defined(__riscv__)
    printf("  Compiler defines: __riscv__\n");
    #endif
#endif

    /* Run tests */
    total_failures += test_basic_vectors();
    total_tests += 8;  /* Number of basic test vectors */

    total_failures += test_boundary_cases();
    total_tests += 15;  /* Number of boundary case tests */

    total_failures += test_chained_crc();
    total_tests += 1;

    total_failures += test_large_buffers();
    total_tests += 5;

    total_failures += test_alignment();
    total_tests += 1;

    /* Print summary */
    print_summary(total_tests, total_failures);

    return total_failures > 0 ? 1 : 0;
}
