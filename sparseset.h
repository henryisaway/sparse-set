#ifndef SPARSESET_H
#define SPARSESET_H

#include <stdint.h>
#include <stdbool.h>

typedef int8_t   i8;
typedef int32_t  i32;
typedef int64_t  i64;
typedef uint8_t  u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uint64_t Entity;

#define PAGE_SIZE 4096
#define INIT_ALLOC_SIZE 16

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

Entity     entity_create(void);
void       entity_reset_counter(void);

SparseSet *sparse_set_create(void);
void       sparse_set_destroy(SparseSet *set);
Position  *sparse_set_get(SparseSet *set, Entity e);
bool       sparse_set_insert(SparseSet *set, Entity e, Position p);

#endif // SPARSESET_H
