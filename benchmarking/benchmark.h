#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "../sparseset.h"
#include <stdbool.h>

typedef struct {
    const char *name;
    u64         attempts;
    u32         inserted;
    u32         duplicates_skipped;
    double      elapsed_seconds;
    double      throughput_mops;

    // Page statistics
    u32         page_count;
    u32         allocated_pages;
    u32         min_page_components;
    u32         max_page_components;
    double      avg_page_components;
    double      page_occupancy_pct;
    u32         fill_buckets[4]; // [0-25%], [26-50%], [51-75%], [76-100%]

    // Memory footprint
    u32         dense_capacity;
    u64         dense_bytes;
    u64         page_dir_bytes;
    u64         page_payload_bytes;
    u64         struct_overhead_bytes;
    u64         total_structure_bytes;
    double      bytes_per_entity;
    i64         live_rss_kib;
    i64         peak_rss_kib;
} BenchmarkMetrics;

typedef Entity (*EntityGeneratorFn)(u64 step, u64 total, void *userdata);

typedef struct {
    const char        *name;
    u64                count;
    EntityGeneratorFn  generator;
    void              *userdata;
    bool               silent;
} BenchmarkConfig;

// Random number helpers
u64 rand64(void);
u64 rand_range(u64 min, u64 max);

// OS Memory helpers
i64 get_peak_mem_usage(void);
i64 get_current_rss_kib(void);

// Benchmark operations
BenchmarkMetrics benchmark_collect_metrics(SparseSet *set, const char *test_name, u64 attempts, double elapsed_seconds);
BenchmarkMetrics benchmark_run(BenchmarkConfig config);

// Reporting
void benchmark_print_metadata(const BenchmarkMetrics *m);
void benchmark_print_comparison(const BenchmarkMetrics *seq, const BenchmarkMetrics *rand_m);

#endif // BENCHMARK_H
