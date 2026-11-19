#ifndef SPARSESET_H
#define SPARSESET_H

#include <stdint.h>
#include <stdbool.h>
#include "type_aliases.h"

typedef uint64_t Entity;

#define PAGE_SIZE 4096
#define INIT_ALLOC_SIZE 16


typedef struct {
    u32   page_count;
    u32   capacity;
    u32   size;
    u64   stride;
    void *data;
    u32  *owners;
    u32 **sparse;
} SparseSet;

Entity     entity_create(void);
void       entity_reset_counter(void);

SparseSet *sparse_set_create(u64 stride);
void sparse_set_destroy(SparseSet *set);
void *sparse_set_get(SparseSet *set, Entity e);
bool sparse_set_insert(SparseSet *set, Entity e, void *data);

#endif // SPARSESET_H
