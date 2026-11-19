#ifndef SPARSESET_H
#define SPARSESET_H

#include <stdint.h>
#include <stdbool.h>
#include "type_aliases.h"

#define PAGE_SIZE 4096
#define INIT_ALLOC_SIZE 16


typedef struct {
    const char *name;
    u32         page_count;
    u32         capacity;
    u32         size;
    u64         stride;
    void       *data;
    u32        *owners;
    u32       **sparse;
} SparseSet;

SparseSet sparse_set_create(const char *name, u64 stride);
void sparse_set_destroy(SparseSet *set);
void *sparse_set_get(SparseSet *set, Entity e);
bool sparse_set_insert(SparseSet *set, Entity e, void *data);
void sparse_set_debug_show(const SparseSet *set);

#endif // SPARSESET_H
