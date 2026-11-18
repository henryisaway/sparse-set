#include "sparseset.h"
#include "benchmark.h"
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_ENTITY_AMT (50ULL * 1000 * 1000)

static Entity sequential_generator(u64 step, u64 total, void *userdata) {
    (void)step;
    (void)total;
    (void)userdata;
    return entity_create();
}

static Entity randomized_generator(u64 step, u64 total, void *userdata) {
    (void)step;
    u64 max_range = (u64)(uintptr_t)userdata;
    if (max_range == 0) max_range = total;
    return rand_range(0, max_range);
}

int main(int argc, char **argv) {
    u64 entity_amt = DEFAULT_ENTITY_AMT;
    if (argc > 1) {
        long long val = atoll(argv[1]);
        if (val > 0) {
            entity_amt = (u64)val;
        }
    }

    if (!setlocale(LC_NUMERIC, "")) {
        setlocale(LC_NUMERIC, "en_GB.UTF-8");
    }

    printf("================================================================================\n");
    printf("   SPARSE SET INSERTION BENCHMARK SUITE\n");
    printf("   Entities: %'lu | Page Size: %d elements (%lu KiB/page)\n",
           (unsigned long)entity_amt, PAGE_SIZE, (unsigned long)(PAGE_SIZE * sizeof(u32))/1024);
    printf("================================================================================\n\n");

    // 1. Sequential ID insertion benchmark
    entity_reset_counter();
    BenchmarkConfig seq_config = {
        .name      = "Sequential IDs Insertion",
        .count     = entity_amt,
        .generator = sequential_generator,
        .userdata  = NULL,
        .silent    = false
    };
    BenchmarkMetrics seq_metrics = benchmark_run(seq_config);

    // 2. Randomized ID insertion benchmark
    BenchmarkConfig rand_config = {
        .name      = "Randomized IDs Insertion",
        .count     = entity_amt,
        .generator = randomized_generator,
        .userdata  = (void *)entity_amt,
        .silent    = false
    };
    BenchmarkMetrics rand_metrics = benchmark_run(rand_config);

    // 3. Side-by-side comparative summary
    benchmark_print_comparison(&seq_metrics, &rand_metrics);

    return 0;
}
