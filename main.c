#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

typedef int8_t i8;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uint64_t Entity;
typedef struct timespec Time;
typedef struct rusage Resources;

#define PAGE_SIZE 4096
#define INIT_ALLOC_SIZE 16
#define ENTITY_AMT (100ULL * 1000 * 1000)

static Entity g_entity_count = 0;

typedef struct {
    u32 x;
    u32 y;
} Position;

typedef struct {
    u32         page_count;
    u32         capacity;
    u32         size;
    Position   *dense;
    u32       **sparse;
} SparseSet;

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
    u64         dense_bytes;
    u64         page_dir_bytes;
    u64         page_payload_bytes;
    u64         struct_overhead_bytes;
    u64         total_structure_bytes;
    double      bytes_per_entity;
    i64         live_rss_kib;
    i64         peak_rss_kib;
} BenchmarkMetrics;

u64 rand64(void) {
    u64 r = 0;
    for (int i = 0; i < 5; i++) {
        // Mask with 0x7FFF to ensure 15 bits regardless of platform.
        r = (r << 15) | (rand() & 0x7FFF);
    }
    return r;
}

u64 rand_range(u64 min, u64 max) {
    u64 range = max - min + 1;
    return (rand64() % range) + min;
}

i64 get_peak_mem_usage(void) {
    Resources r;
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

Entity entity_create(void) {
    return g_entity_count++;
}

SparseSet *sparse_set_create(void) {
    SparseSet *set  = (SparseSet *)calloc(1, sizeof(SparseSet));

    set->capacity   = INIT_ALLOC_SIZE;
    set->page_count = 1;
    set->size       = 0;
    set->dense      = calloc(set->capacity, sizeof(Position));
    set->sparse     = calloc(set->page_count, sizeof(u32 *));
    *(set->sparse)  = NULL;

    return set;
}

void sparse_set_destroy(SparseSet *set) {
    for (u32 i = 0; i < set->page_count; i++) {
        if (set->sparse[i] != NULL)
            free(set->sparse[i]);
    }
    free(set->sparse);
    free(set->dense);
    free(set);
}

Position *sparse_set_get(SparseSet *set, Entity e) {
    u32 page_index  = (u32)(e / PAGE_SIZE);
    u32 page_offset = (u32)(e % PAGE_SIZE);

    // Page doesn't exist
    if (page_index >= set->page_count)
        return NULL;

    if (set->sparse[page_index] == NULL)
        return NULL;

    u32 dense_index = set->sparse[page_index][page_offset];
    if (dense_index == UINT32_MAX) {
        return NULL;
    }

    Position *position = &set->dense[dense_index];

    return position;
}

void sparse_set_insert(SparseSet *set, Entity e, Position p) {
    u32 page_index  = (u32)(e / PAGE_SIZE);
    u32 page_offset = (u32)(e % PAGE_SIZE);

    if (page_index >= set->page_count) {
        // Not enough valid page pointers.
        u32 new_page_count          = page_index + 1;
        u32 **new_sparse_page_array = realloc(set->sparse, new_page_count * sizeof(u32 *));

        if (new_sparse_page_array) {
            set->sparse = new_sparse_page_array;
        } else {
            exit(1);
        }

        for (u32 i = set->page_count; i < new_page_count; i++) {
            set->sparse[i] = NULL;
        }

        set->page_count = new_page_count;
    }

    if (set->sparse[page_index] == NULL) {
        // Page hasn't been allocated yet.
        u32 *new_sparse_page = malloc(PAGE_SIZE * sizeof(u32));
        if (new_sparse_page) {
            set->sparse[page_index] = new_sparse_page;
        } else {
            exit(1);
        }

        memset(set->sparse[page_index], UINT8_MAX, PAGE_SIZE * sizeof(u32));
    }

    // By this point, the page is guaranteed to exist.

    if (set->sparse[page_index][page_offset] != UINT32_MAX) {
        // Entity is already in the sparse set
        return;
    }

    if (set->size == set->capacity) {
        // Need to increase the capacity of dense array.

        Position* new_dense_array = realloc(set->dense, 2 * set->capacity * sizeof(Position));
        if (new_dense_array) {
            set->dense = new_dense_array;
        } else {
            exit(1);
        }
        
        set->capacity *= 2;
    }

    set->dense[set->size]                = p;
    set->sparse[page_index][page_offset] = set->size++;
}

BenchmarkMetrics collect_metrics(SparseSet *set, const char *test_name, u64 attempts, double elapsed_seconds) {
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

    // Exact structural memory calculation
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

void print_benchmark_metadata(const BenchmarkMetrics *m) {
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
    printf("    Total Struct Memory   : %8.2f MiB (%'lu B)\n",
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
    printf("      [  0%% -  25%% ]     : %'7u pages (%5.1f%%)\n",
           m->fill_buckets[0], m->allocated_pages ? (double)m->fill_buckets[0] / m->allocated_pages * 100.0 : 0.0);
    printf("      [ 26%% -  50%% ]     : %'7u pages (%5.1f%%)\n",
           m->fill_buckets[1], m->allocated_pages ? (double)m->fill_buckets[1] / m->allocated_pages * 100.0 : 0.0);
    printf("      [ 51%% -  75%% ]     : %'7u pages (%5.1f%%)\n",
           m->fill_buckets[2], m->allocated_pages ? (double)m->fill_buckets[2] / m->allocated_pages * 100.0 : 0.0);
    printf("      [ 76%% - 100%% ]     : %'7u pages (%5.1f%%)\n",
           m->fill_buckets[3], m->allocated_pages ? (double)m->fill_buckets[3] / m->allocated_pages * 100.0 : 0.0);
    printf("================================================================================\n\n");
}

void sparse_set_print_metadata(SparseSet *set) {
    BenchmarkMetrics m = collect_metrics(set, "Sparse Set Metadata", set->size, 0.0);
    print_benchmark_metadata(&m);
}

void print_benchmark_comparison(const BenchmarkMetrics *seq, const BenchmarkMetrics *rand_m) {
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

int main(int argc, char **argv) {
    u64 entity_amt = ENTITY_AMT;
    if (argc > 1) {
        long long val = atoll(argv[1]);
        if (val > 0) {
            entity_amt = (u64)val;
        }
    }

    if (!setlocale(LC_NUMERIC, "")) {
        setlocale(LC_NUMERIC, "en_GB.UTF-8");
    }

    printf("Starting SparseSet Benchmark (ENTITY_AMT = %'lu, PAGE_SIZE = %d)\n\n",
           (unsigned long)entity_amt, PAGE_SIZE);

    Time start, end;

    // Sequential ID insertions
    SparseSet *seq_set = sparse_set_create();

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (u64 i = 0; i < entity_amt; i++) {
        Entity ent = entity_create();
        sparse_set_insert(seq_set, ent, (Position) {
                                            .x = (u32)(rand() % 500),
                                            .y = (u32)(rand() % 500)
                                        });
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double seq_elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    BenchmarkMetrics seq_metrics = collect_metrics(seq_set, "Sequential IDs Insertion", entity_amt, seq_elapsed);
    print_benchmark_metadata(&seq_metrics);
    sparse_set_destroy(seq_set);

    // Randomised ID insertions
    SparseSet *rand_set = sparse_set_create();

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (u64 i = 0; i < entity_amt; i++) {
        Entity ent = rand_range(0, entity_amt);
        sparse_set_insert(rand_set, ent, (Position) {
                                             .x = (u32)(rand() % 500),
                                             .y = (u32)(rand() % 500)
                                         });
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double rand_elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    BenchmarkMetrics rand_metrics = collect_metrics(rand_set, "Randomized IDs Insertion", entity_amt, rand_elapsed);
    print_benchmark_metadata(&rand_metrics);
    sparse_set_destroy(rand_set);

    print_benchmark_comparison(&seq_metrics, &rand_metrics);

    return 0;
}