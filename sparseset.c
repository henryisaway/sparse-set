#include "sparseset.h"
#include <stdlib.h>
#include <string.h>

static Entity g_entity_count = 0;

Entity entity_create(void) {
    return g_entity_count++;
}

void entity_reset_counter(void) {
    g_entity_count = 0;
}

SparseSet *sparse_set_create(void) {
    SparseSet *set  = (SparseSet *)calloc(1, sizeof(SparseSet));
    if (!set) exit(1);

    set->capacity   = INIT_ALLOC_SIZE;
    set->page_count = 1;
    set->size       = 0;
    set->dense      = (Position *)calloc(set->capacity, sizeof(Position));
    set->sparse     = (u32 **)calloc(set->page_count, sizeof(u32 *));

    if (!set->dense || !set->sparse) {
        free(set->dense);
        free(set->sparse);
        free(set);
        exit(1);
    }

    *(set->sparse)  = NULL;

    return set;
}

void sparse_set_destroy(SparseSet *set) {
    if (!set) return;

    for (u32 i = 0; i < set->page_count; i++) {
        if (set->sparse[i] != NULL) {
            free(set->sparse[i]);
        }
    }
    free(set->sparse);
    free(set->dense);
    free(set);
}

Position *sparse_set_get(SparseSet *set, Entity e) {
    u32 page_index  = (u32)(e / PAGE_SIZE);
    u32 page_offset = (u32)(e % PAGE_SIZE);

    if (page_index >= set->page_count)
        return NULL;

    if (set->sparse[page_index] == NULL)
        return NULL;

    u32 dense_index = set->sparse[page_index][page_offset];
    if (dense_index == UINT32_MAX) {
        return NULL;
    }

    return &set->dense[dense_index];
}

bool sparse_set_insert(SparseSet *set, Entity e, Position p) {
    u32 page_index  = (u32)(e / PAGE_SIZE);
    u32 page_offset = (u32)(e % PAGE_SIZE);

    if (page_index >= set->page_count) {
        u32 new_page_count = page_index + 1;
        u32 **new_sparse_page_array = (u32 **)realloc(set->sparse, new_page_count * sizeof(u32 *));

        if (!new_sparse_page_array) {
            exit(1);
        }

        set->sparse = new_sparse_page_array;

        for (u32 i = set->page_count; i < new_page_count; i++) {
            set->sparse[i] = NULL;
        }

        set->page_count = new_page_count;
    }

    if (set->sparse[page_index] == NULL) {
        u32 *new_sparse_page = (u32 *)malloc(PAGE_SIZE * sizeof(u32));
        if (!new_sparse_page) {
            exit(1);
        }

        set->sparse[page_index] = new_sparse_page;
        memset(set->sparse[page_index], UINT8_MAX, PAGE_SIZE * sizeof(u32));
    }

    if (set->sparse[page_index][page_offset] != UINT32_MAX) {
        // Entity is already present
        return false;
    }

    if (set->size == set->capacity) {
        u32 new_capacity = set->capacity * 2;
        Position *new_dense_array = (Position *)realloc(set->dense, new_capacity * sizeof(Position));
        if (!new_dense_array) {
            exit(1);
        }

        set->dense = new_dense_array;
        set->capacity = new_capacity;
    }

    set->dense[set->size]                = p;
    set->sparse[page_index][page_offset] = set->size++;
    return true;
}
