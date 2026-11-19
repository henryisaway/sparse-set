#include "sparseset.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "type_aliases.h"

static Entity g_entity_count = 0;

Entity entity_create(void) {
    return g_entity_count++;
}

void entity_reset_counter(void) {
    g_entity_count = 0;
}

SparseSet *sparse_set_create(u64 stride) {
    SparseSet *set  = (SparseSet *)calloc(1, sizeof(SparseSet));
    if (!set) exit(1);

    set->capacity   = INIT_ALLOC_SIZE;
    set->page_count = 1;
    set->size       = 0;
    set->stride     = stride;
    set->data       = calloc(set->capacity, stride);
    set->owners     = calloc(set->capacity, sizeof(u32));
    set->sparse     = (u32 **)calloc(set->page_count, sizeof(u32 *));

    if (!set->data || !set->sparse) {
        free(set->data);
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
    free(set->owners);
    free(set->data);
    free(set);
}

void *sparse_set_get(SparseSet *set, Entity e) {
    u32 page_index  = (u32)(e / PAGE_SIZE);
    u32 page_offset = (u32)(e % PAGE_SIZE);

    if (page_index >= set->page_count)
        return NULL;

    if (set->sparse[page_index] == NULL)
        return NULL;

    u32 data_index = set->sparse[page_index][page_offset];
    if (data_index == UINT32_MAX) {
        return NULL;
    }

    return (u8*)(set->data) + (data_index * set->stride);
}

bool sparse_set_insert(SparseSet *set, Entity e, void *data) {
    u32 page_index  = (u32)(e >> 12);
    u32 page_offset = (u32)(e % 4095);

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

    u32 data_index = set->sparse[page_index][page_offset];
    if (data_index != UINT32_MAX) {
        // Entity is already present
        return false;
    }

    if (set->size == set->capacity) {
        u32 new_capacity = set->capacity * 2;
        void *new_data_array = realloc(set->data, new_capacity * set->stride);
        if (!new_data_array) {
            exit(1);
        }

        void *new_owners_array = realloc(set->owners, new_capacity * sizeof(u32));
        if (!new_owners_array) {
            exit(1);
        }

        set->data = new_data_array;
        set->capacity = new_capacity;
    }

    u8 *dest = (u8*)(set->data) + (data_index * set->stride);
    memcpy(dest, data, set->stride);
    set->owners[data_index] = e;
    set->sparse[page_index][page_offset] = set->size++;
    return true;
}
