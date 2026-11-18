#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uint64_t Entity;

#define PAGE_SIZE 16
#define INIT_ALLOC_SIZE 16

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

void sparse_set_print(SparseSet *set) {

    printf("ALL PAGES (SPARSE SET) ===========================\n");
    for (int i = 0; i < set->page_count; i++) {
        
        if (set->sparse[i] != NULL) {
            printf("Page #%d:\n", i);

            for (int j = 0; j < PAGE_SIZE; j++) {
                if (set->sparse[i][j] != UINT32_MAX)
                    printf("- Entity #%d exists.\n", PAGE_SIZE * i + j);
            }
        }
    }

    printf("\nPOSITIONS (DENSE SET) ============================\n");
    for (int i = 0; i < set->size; i++) {
        
        printf("{.x = %d, .y = %d}\n", set->dense[i].x, set->dense[i].y);
    }
}

int main(void) {
    SparseSet *set = sparse_set_create();

    for (int i = 0; i < 50; i++) {
        Entity ent = entity_create();
        sparse_set_insert(set, ent, (Position){
                                        .x = rand() % 500,
                                        .y = rand() % 500
                                    });
    }

    sparse_set_print(set);

    sparse_set_destroy(set);
    return 0;
}