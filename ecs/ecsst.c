#include "ecs.h"
#include "../benchmarking/benchmark.h"
#include "../type_aliases.h"

#include <locale.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// This test was AI generated.
// ── Component definitions ─────────────────────────────────────────────────────

typedef struct {
    f32 x;
    f32 y;
    f32 z;
} Position;

typedef struct {
    f32 x;
    f32 y;
    f32 z;
} Velocity;

typedef struct {
    u32 hp;
    u32 max_hp;
} Health;

// ── Default configurations ────────────────────────────────────────────────────

#define DEFAULT_FRAMES 500

static double elapsed_since(const struct timespec *start) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)(now.tv_sec - start->tv_sec) +
           (double)(now.tv_nsec - start->tv_nsec) / 1e9;
}

// ── Phase 1: Sequential Bulk Insert ──────────────────────────────────────────

static void phase_sequential(u64 entity_count) {
    printf("\n  ── Phase 1: Sequential Bulk Insert (%'llu entities) ──\n",
           (unsigned long long)entity_count);

    ecs_init();
    TypeID pos_id = register_component(Position);
    TypeID vel_id = register_component(Velocity);
    TypeID hp_id  = register_component(Health);

    struct timespec t0;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    entity_reset_ids();
    for (u64 i = 0; i < entity_count; i++) {
        Entity e = entity_create();
        entity_bind_component(pos_id, e, &(Position){.x = 0.0f, .y = 0.0f, .z = 0.0f});
        entity_bind_component(vel_id, e, &(Velocity){.x = 1.0f, .y = 0.5f, .z = 0.0f});
        entity_bind_component(hp_id,  e, &(Health){.hp = 100, .max_hp = 100});
    }

    double elapsed = elapsed_since(&t0);
    u64 total_ops = entity_count * 3;

    printf("    Insert Attempts       : %'llu\n", (unsigned long long)total_ops);
    printf("    Elapsed Time          : %.4f seconds\n", elapsed);
    printf("    Throughput            : %.2f M bind-ops/sec\n",
           elapsed > 0.0 ? ((double)total_ops / elapsed / 1e6) : 0.0);
    printf("    Process Live RSS      : %8.2f MiB\n", (double)get_current_rss_kib() / 1024.0);
    printf("    Process Peak RSS      : %8.2f MiB\n", (double)get_peak_mem_usage() / 1024.0);

    // Free all allocated memory for this phase
    ecs_shutdown();
    malloc_trim(0);
    printf("    Post-Cleanup Live RSS : %8.2f MiB\n", (double)get_current_rss_kib() / 1024.0);
}

// ── Phase 2: Random-ID Sparse Insert ─────────────────────────────────────────

static void phase_random(u64 attempts) {
    printf("\n  ── Phase 2: Random-ID Sparse Insert (%'llu attempts) ──\n",
           (unsigned long long)attempts);

    ecs_init();
    TypeID pos_id = register_component(Position);
    TypeID vel_id = register_component(Velocity);
    TypeID hp_id  = register_component(Health);

    u64 bound = attempts * 2; // ~50% duplicate target
    u64 inserted = 0;
    u64 duplicates = 0;

    struct timespec t0;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (u64 i = 0; i < attempts; i++) {
        Entity e = (Entity)rand_range(0, bound);
        bool ok = entity_bind_component(pos_id, e, &(Position){
            .x = (f32)(i % 500),
            .y = 0.0f,
            .z = 0.0f
        });

        if (ok) {
            entity_bind_component(vel_id, e, &(Velocity){.x = 1.0f, .y = 1.0f, .z = 1.0f});
            entity_bind_component(hp_id,  e, &(Health){.hp = 50, .max_hp = 100});
            inserted++;
        } else {
            duplicates++;
        }
    }

    double elapsed = elapsed_since(&t0);

    printf("    Attempts              : %'llu\n", (unsigned long long)attempts);
    printf("    Unique Entities Added : %'llu\n", (unsigned long long)inserted);
    printf("    Duplicates Skipped    : %'llu (%.2f%%)\n",
           (unsigned long long)duplicates,
           attempts ? ((double)duplicates / (double)attempts * 100.0) : 0.0);
    printf("    Elapsed Time          : %.4f seconds\n", elapsed);
    printf("    Throughput            : %.2f M attempts/sec\n",
           elapsed > 0.0 ? ((double)attempts / elapsed / 1e6) : 0.0);
    printf("    Process Live RSS      : %8.2f MiB\n", (double)get_current_rss_kib() / 1024.0);
    printf("    Process Peak RSS      : %8.2f MiB\n", (double)get_peak_mem_usage() / 1024.0);

    // Free all allocated memory for this phase
    ecs_shutdown();
    malloc_trim(0);
    printf("    Post-Cleanup Live RSS : %8.2f MiB\n", (double)get_current_rss_kib() / 1024.0);
}

// ── Phase 3: System Simulation ────────────────────────────────────────────────

static void phase_simulate(u64 entity_count, u32 frames) {
    printf("\n  ── Phase 3: System Simulation (%u frames × %'llu entities) ──\n",
           frames, (unsigned long long)entity_count);

    ecs_init();
    TypeID pos_id = register_component(Position);
    TypeID vel_id = register_component(Velocity);
    TypeID hp_id  = register_component(Health);

    entity_reset_ids();
    for (u64 i = 0; i < entity_count; i++) {
        Entity e = entity_create();
        entity_bind_component(pos_id, e, &(Position){.x = 0.0f, .y = 0.0f, .z = 0.0f});
        entity_bind_component(vel_id, e, &(Velocity){.x = 1.0f, .y = 0.5f, .z = 0.25f});
        // 10% of entities are simulated dead/critical to test filter skipping
        u32 hp = (i % 10 == 0) ? 5 : 100;
        entity_bind_component(hp_id,  e, &(Health){.hp = hp, .max_hp = 100});
    }

    struct timespec t0;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    u64 updates = 0;
    for (u32 f = 0; f < frames; f++) {
        for (u64 e = 0; e < entity_count; e++) {
            Health *h = entity_get_component(hp_id, e);
            if (!h || h->hp < 10) continue;

            Position *p = entity_get_component(pos_id, e);
            Velocity *v = entity_get_component(vel_id, e);
            if (!p || !v) continue;

            p->x += v->x;
            p->y += v->y;
            p->z += v->z;
            updates++;
        }
    }

    double elapsed = elapsed_since(&t0);

    printf("    Simulated Frames      : %u\n", frames);
    printf("    Components Updated    : %'llu\n", (unsigned long long)updates);
    printf("    Elapsed Time          : %.4f seconds\n", elapsed);
    printf("    Simulation Rate       : %.2f frames/sec\n",
           elapsed > 0.0 ? ((double)frames / elapsed) : 0.0);
    printf("    Component Read Rate   : %.2f M lookups/sec\n",
           elapsed > 0.0 ? ((double)(updates * 3) / elapsed / 1e6) : 0.0);
    printf("    Process Live RSS      : %8.2f MiB\n", (double)get_current_rss_kib() / 1024.0);
    printf("    Process Peak RSS      : %8.2f MiB\n", (double)get_peak_mem_usage() / 1024.0);

    // Free all allocated memory for this phase
    ecs_shutdown();
    malloc_trim(0);
    printf("    Post-Cleanup Live RSS : %8.2f MiB\n", (double)get_current_rss_kib() / 1024.0);
}

// ── Test Runner ───────────────────────────────────────────────────────────────

static void run_test_suite(const char *label, u64 entity_count, u32 frames) {
    printf("\n################################################################################\n");
    printf("  TEST SUITE: %s\n", label);
    printf("  Entities: %'llu | Frames: %u\n", (unsigned long long)entity_count, frames);
    printf("  Component Types: Position (%zu B), Velocity (%zu B), Health (%zu B)\n",
           sizeof(Position), sizeof(Velocity), sizeof(Health));
    printf("################################################################################\n");

    phase_sequential(entity_count);
    phase_random(entity_count);
    phase_simulate(entity_count, frames);
}

// ── Main Entry Point ──────────────────────────────────────────────────────────

int main(int argc, char **argv) {
    if (!setlocale(LC_NUMERIC, "")) {
        setlocale(LC_NUMERIC, "en_GB.UTF-8");
    }

    srand(42);

    u32 frames = DEFAULT_FRAMES;

    printf("================================================================================\n");
    printf("                         ECS INTERFACE STRESS TEST\n");
    printf("================================================================================\n");

    if (argc > 1) {
        const char *arg = argv[1];
        if (argc > 2) {
            int f = atoi(argv[2]);
            if (f > 0) frames = (u32)f;
        }

        if (strcmp(arg, "A") == 0 || strcmp(arg, "100k") == 0) {
            run_test_suite("Run A (Realistic Workload)", 100000ULL, frames);
        } else if (strcmp(arg, "B") == 0 || strcmp(arg, "1m") == 0) {
            run_test_suite("Run B (High-Load Workload)", 1000000ULL, frames);
        } else if (strcmp(arg, "C") == 0 || strcmp(arg, "10m") == 0) {
            run_test_suite("Run C (Extreme Stress Test)", 10000000ULL, frames);
        } else {
            long long custom_count = atoll(arg);
            if (custom_count > 0) {
                run_test_suite("Custom Workload", (u64)custom_count, frames);
            } else {
                fprintf(stderr, "Usage: %s [A|100k | B|1m | C|10m | <entity_count>] [frames]\n", argv[0]);
                return 1;
            }
        }
    } else {
        // Run all three suites sequentially
        run_test_suite("Run A (Realistic Workload)",  100000ULL,   frames);
        run_test_suite("Run B (High-Load Workload)",  1000000ULL,  frames);
        run_test_suite("Run C (Extreme Stress Test)", 10000000ULL, frames);
    }

    printf("\n================================================================================\n");
    printf("  All stress tests completed successfully.\n");
    printf("================================================================================\n");

    return 0;
}
