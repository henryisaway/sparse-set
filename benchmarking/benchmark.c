#include "benchmark.h"
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

u64 rand64(void) {
    u64 r = 0;
    for (int i = 0; i < 5; i++) {
        r = (r << 15) | (rand() & 0x7FFF);
    }
    return r;
}

u64 rand_range(u64 min, u64 max) {
    u64 range = max - min + 1;
    return (rand64() % range) + min;
}

i64 get_peak_mem_usage(void) {
    struct rusage r;
    if (getrusage(RUSAGE_SELF, &r) == 0)
        return r.ru_maxrss;
    return -1;
}

i64 get_current_rss_kib(void) {
    FILE *fp = fopen("/proc/self/statm", "r");
    if (!fp) return -1;

    long total_pages = 0, resident_pages = 0;
    if (fscanf(fp, "%ld %ld", &total_pages, &resident_pages) != 2) {
        fclose(fp);
        return -1;
    }
    fclose(fp);

    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) page_size = 4096;

    return (i64)((resident_pages * page_size) / 1024);
}

BenchmarkMetrics benchmark_collect_metrics(SparseSet *set, const char *test_name, u64 attempts, double elapsed_seconds) {
    BenchmarkMetrics m;
    memset(&m, 0, sizeof(m));

    m.name = test_name;
    m.attempts = attempts;
    m.inserted = set->size;
    m.duplicates_skipped = (attempts > set->size) ? (u32)(attempts - set->size) : 0;
    m.elapsed_seconds = elapsed_seconds;
    m.throughput_mops = (elapsed_seconds > 0.0) ? ((double)attempts / elapsed_seconds / 1e6) : 0.0;

    m.page_count = set->page_count;
    m.allocated_pages = 0;
    m.min_page_components = PAGE_SIZE + 1;
    m.max_page_components = 0;

    u64 total_components = 0;
    for (u32 page = 0; page < set->page_count; page++) {
        if (set->sparse[page] != NULL) {
            m.allocated_pages++;
            u32 page_comps = 0;
            for (u32 offset = 0; offset < PAGE_SIZE; offset++) {
                if (set->sparse[page][offset] != UINT32_MAX) {
                    page_comps++;
                }
            }
            if (page_comps < m.min_page_components) m.min_page_components = page_comps;
            if (page_comps > m.max_page_components) m.max_page_components = page_comps;
            total_components += page_comps;

            double pct = (double)page_comps / PAGE_SIZE * 100.0;
            if (pct <= 25.0) {
                m.fill_buckets[0]++;
            } else if (pct <= 50.0) {
                m.fill_buckets[1]++;
            } else if (pct <= 75.0) {
                m.fill_buckets[2]++;
            } else {
                m.fill_buckets[3]++;
            }
        }
    }

    if (m.allocated_pages == 0) {
        m.min_page_components = 0;
        m.avg_page_components = 0.0;
        m.page_occupancy_pct = 0.0;
    } else {
        m.avg_page_components = (double)total_components / m.allocated_pages;
        m.page_occupancy_pct = ((double)total_components / ((u64)m.allocated_pages * PAGE_SIZE)) * 100.0;
    }

    m.dense_bytes           = (u64)set->capacity * sizeof(Position);
    m.page_dir_bytes        = (u64)set->page_count * sizeof(u32 *);
    m.page_payload_bytes    = (u64)m.allocated_pages * PAGE_SIZE * sizeof(u32);
    m.struct_overhead_bytes = sizeof(SparseSet);
    m.total_structure_bytes = m.dense_bytes + m.page_dir_bytes + m.page_payload_bytes + m.struct_overhead_bytes;

    m.bytes_per_entity = (set->size > 0) ? ((double)m.total_structure_bytes / set->size) : 0.0;
    m.live_rss_kib = get_current_rss_kib();
    m.peak_rss_kib = get_peak_mem_usage();

    return m;
}

BenchmarkMetrics benchmark_run(BenchmarkConfig config) {
    SparseSet *set = sparse_set_create();

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (u64 i = 0; i < config.count; i++) {
        Entity ent = config.generator(i, config.count, config.userdata);
        Position pos = {
            .x = (u32)(rand() % 500),
            .y = (u32)(rand() % 500)
        };
        sparse_set_insert(set, ent, pos);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    BenchmarkMetrics metrics = benchmark_collect_metrics(set, config.name, config.count, elapsed);

    if (!config.silent) {
        benchmark_print_metadata(&metrics);
    }

    sparse_set_destroy(set);
    return metrics;
}

void benchmark_print_metadata(const BenchmarkMetrics *m) {
    if (!setlocale(LC_NUMERIC, "")) {
        setlocale(LC_NUMERIC, "en_GB.UTF-8");
    }

    printf("================================================================================\n");
    printf("  METADATA: %s\n", m->name);
    printf("================================================================================\n");
    printf("  [Execution Performance]\n");
    printf("    Insert Attempts       : %'lu\n", (unsigned long)m->attempts);
    printf("    Entities Inserted     : %'u\n", m->inserted);
    printf("    Duplicates Skipped    : %'u (%.2f%%)\n", m->duplicates_skipped,
           m->attempts ? ((double)m->duplicates_skipped / m->attempts * 100.0) : 0.0);
    printf("    Elapsed Time          : %.4f seconds\n", m->elapsed_seconds);
    printf("    Throughput            : %.2f M ops/sec\n\n", m->throughput_mops);

    printf("  [Memory Footprint]\n");
    printf("    Dense Buffer Cap      : %'u entries (%8.2f MiB | %'lu B)\n",
           (u32)(m->dense_bytes / sizeof(Position)),
           (double)m->dense_bytes / (1024.0 * 1024.0), (unsigned long)m->dense_bytes);
    printf("    Page Table Slots      : %'u slots   (%8.2f MiB | %'lu B)\n",
           m->page_count,
           (double)m->page_dir_bytes / (1024.0 * 1024.0), (unsigned long)m->page_dir_bytes);
    printf("    Allocated Pages Mem   : %'u pages   (%8.2f MiB | %'lu B)\n",
           m->allocated_pages,
           (double)m->page_payload_bytes / (1024.0 * 1024.0), (unsigned long)m->page_payload_bytes);
    printf("    Total Struct Memory   :%8.2f MiB (%'lu B)\n",
           (double)m->total_structure_bytes / (1024.0 * 1024.0), (unsigned long)m->total_structure_bytes);
    printf("    Average Memory/Entity : %.2f bytes/entity\n", m->bytes_per_entity);
    printf("    Process Live RSS      : %8.2f MiB (%'ld KiB)\n",
           (double)m->live_rss_kib / 1024.0, (long)m->live_rss_kib);
    printf("    Process Peak RSS      : %8.2f MiB (%'ld KiB)\n\n",
           (double)m->peak_rss_kib / 1024.0, (long)m->peak_rss_kib);

    printf("  [Page Utilization (PAGE_SIZE = %d)]\n", PAGE_SIZE);
    printf("    Directory Entries     : %'u slots\n", m->page_count);
    printf("    Allocated Pages Count : %'u (%.2f%% of directory)\n",
           m->allocated_pages, m->page_count ? ((double)m->allocated_pages / m->page_count * 100.0) : 0.0);
    printf("    Components / Page     : Min: %'u | Avg: %.1f | Max: %'u (Fullness: %.1f%%)\n",
           m->min_page_components, m->avg_page_components, m->max_page_components, m->page_occupancy_pct);
    printf("    Page Fill Buckets     :\n");
    printf("      [  0%% -  25%% ]      : %'7u pages (%5.1f%%)\n",
           m->fill_buckets[0], m->allocated_pages ? (double)m->fill_buckets[0] / m->allocated_pages * 100.0 : 0.0);
    printf("      [ 26%% -  50%% ]      : %'7u pages (%5.1f%%)\n",
           m->fill_buckets[1], m->allocated_pages ? (double)m->fill_buckets[1] / m->allocated_pages * 100.0 : 0.0);
    printf("      [ 51%% -  75%% ]      : %'7u pages (%5.1f%%)\n",
           m->fill_buckets[2], m->allocated_pages ? (double)m->fill_buckets[2] / m->allocated_pages * 100.0 : 0.0);
    printf("      [ 76%% - 100%% ]      : %'7u pages (%5.1f%%)\n",
           m->fill_buckets[3], m->allocated_pages ? (double)m->fill_buckets[3] / m->allocated_pages * 100.0 : 0.0);
    printf("================================================================================\n\n");
}

void benchmark_print_comparison(const BenchmarkMetrics *seq, const BenchmarkMetrics *rand_m) {
    if (!setlocale(LC_NUMERIC, "")) {
        setlocale(LC_NUMERIC, "en_GB.UTF-8");
    }

    printf("################################################################################\n");
    printf("                       SPARSE SET BENCHMARK COMPARISON                          \n");
    printf("################################################################################\n");
    printf("%-26s | %-18s | %-18s | %-12s\n", "Metric", "Sequential", "Randomized", "Ratio (R/S)");
    printf("---------------------------+--------------------+--------------------+--------------\n");
    printf("%-26s | %'18lu | %'18lu | %10.2fx\n", "Insert Attempts",
           (unsigned long)seq->attempts, (unsigned long)rand_m->attempts,
           seq->attempts ? (double)rand_m->attempts / seq->attempts : 0.0);
    printf("%-26s | %'18u | %'18u | %10.2fx\n", "Entities Stored",
           seq->inserted, rand_m->inserted,
           seq->inserted ? (double)rand_m->inserted / seq->inserted : 0.0);
    printf("%-26s | %'18u | %'18u | %10s\n", "Duplicates Skipped",
           seq->duplicates_skipped, rand_m->duplicates_skipped, "-");
    printf("%-26s | %17.4fs | %17.4fs | %10.2fx\n", "Elapsed Time",
           seq->elapsed_seconds, rand_m->elapsed_seconds,
           seq->elapsed_seconds ? rand_m->elapsed_seconds / seq->elapsed_seconds : 0.0);
    printf("%-26s | %14.2f M/s | %14.2f M/s | %10.2fx\n", "Throughput",
           seq->throughput_mops, rand_m->throughput_mops,
           seq->throughput_mops ? rand_m->throughput_mops / seq->throughput_mops : 0.0);
    printf("---------------------------+--------------------+--------------------+--------------\n");
    printf("%-26s | %'18u | %'18u | %10.2fx\n", "Directory Slots",
           seq->page_count, rand_m->page_count,
           seq->page_count ? (double)rand_m->page_count / seq->page_count : 0.0);
    printf("%-26s | %'18u | %'18u | %10.2fx\n", "Allocated Pages",
           seq->allocated_pages, rand_m->allocated_pages,
           seq->allocated_pages ? (double)rand_m->allocated_pages / seq->allocated_pages : 0.0);
    printf("%-26s | %17.1f%% | %17.1f%% | %10.2fx\n", "Page Occupancy",
           seq->page_occupancy_pct, rand_m->page_occupancy_pct,
           seq->page_occupancy_pct ? rand_m->page_occupancy_pct / seq->page_occupancy_pct : 0.0);
    printf("%-26s | %14.2f MiB | %14.2f MiB | %10.2fx\n", "Dense Buffer Mem",
           (double)seq->dense_bytes / (1024.0 * 1024.0), (double)rand_m->dense_bytes / (1024.0 * 1024.0),
           seq->dense_bytes ? (double)rand_m->dense_bytes / seq->dense_bytes : 0.0);
    printf("%-26s | %14.2f MiB | %14.2f MiB | %10.2fx\n", "Sparse Page Mem",
           (double)seq->page_payload_bytes / (1024.0 * 1024.0), (double)rand_m->page_payload_bytes / (1024.0 * 1024.0),
           seq->page_payload_bytes ? (double)rand_m->page_payload_bytes / seq->page_payload_bytes : 0.0);
    printf("%-26s | %14.2f MiB | %14.2f MiB | %10.2fx\n", "Total Struct Mem",
           (double)seq->total_structure_bytes / (1024.0 * 1024.0), (double)rand_m->total_structure_bytes / (1024.0 * 1024.0),
           seq->total_structure_bytes ? (double)rand_m->total_structure_bytes / seq->total_structure_bytes : 0.0);
    printf("%-26s | %14.2f B/e | %14.2f B/e | %10.2fx\n", "Bytes / Entity",
           seq->bytes_per_entity, rand_m->bytes_per_entity,
           seq->bytes_per_entity ? rand_m->bytes_per_entity / seq->bytes_per_entity : 0.0);
    printf("%-26s | %14.2f MiB | %14.2f MiB | %10.2fx\n", "OS Live RSS",
           (double)seq->live_rss_kib / 1024.0, (double)rand_m->live_rss_kib / 1024.0,
           (seq->live_rss_kib > 0) ? (double)rand_m->live_rss_kib / seq->live_rss_kib : 0.0);
    printf("################################################################################\n");
}
