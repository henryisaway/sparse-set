#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>

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
#define ENTITY_AMT 100 * 1000 * 1000

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

i64 get_peak_mem_usage() {
    Resources r;
    if (getrusage(RUSAGE_SELF, &r) == 0)
        return r.ru_maxrss;

    return -1;
}

Entity entity_create(void) {
    return g_entity_count++;
}

SparseSet *sparse_set_create(void) {
    SparseSet *set  = (SparseSet *)calloc(1, sizeof(SparseSet));

    set->capacity   = INIT_ALLOC_SIZE;
    set->page_count = 1;
    set->size       = 0;
    set->dense      = calloc(set->capacity, sizeof (Position));
    set->sparse     = calloc(set->page_count, sizeof(u32 *));
    *(set->sparse)  = NULL;

    return set;
}

void sparse_set_destroy(SparseSet *set) {
    for (u32 i = 0; i < set->page_count; i++) {
        if (set->sparse[i] != NULL)
            free(set->sparse[i]);
    }
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

void sparse_set_print_metadata(SparseSet *set) {
    setlocale(LC_NUMERIC, "en_GB");
    printf("METADATA =========================================\n");
    printf("Size: %'d (%'ld)MiB\n", set->size, get_peak_mem_usage() / 1024);
    printf("Capacity: %'d\n", set->capacity);
    printf("%'d allocated pages.\n", set->page_count);
    printf("\nALL PAGES (SPARSE SET) ===========================\n");
    for (int page = 0; page < set->page_count; page++) {
        
        if (set->sparse[page] != NULL) {
            printf("Page #%'d allocated with ", page);

            u32 amt_components = 0;
            for (int offset = 0; offset < PAGE_SIZE; offset++) {
                if (set->sparse[page][offset] != UINT32_MAX)
                    amt_components++;
            }

            printf("%'d components.\n", amt_components);
        }
    }



}

int main(void) {

    Time start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    SparseSet *set = sparse_set_create();

    for (int i = 0; i < ENTITY_AMT; i++) {
        Entity ent = rand_range(0, ENTITY_AMT);
        sparse_set_insert(set, ent, (Position) {
                                        .x = rand() % 500,
                                        .y = rand() % 500
                                    });
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1e9;
    printf("Elapsed time: %f seconds.\n\n", elapsed);
    sparse_set_print_metadata(set);
    
    sparse_set_destroy(set);

    return 0;
}