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

/** @brief Creates a SparseSet instance.
 * @param[in] name The string literal containing the name of the set.
 * @param[in] stride The size in bytes of each element in the set.
 * @return A copy of the created sparse set.
 */
SparseSet sparse_set_create(const char *name, u64 stride);

/** @brief Destroys a SparseSet instance.
 * @param[in] set The sparse set to be destroyed.
 */
void sparse_set_destroy(SparseSet *set);

/** @brief Retrieves a pointer to the data bound to an entity.
 * @param[in] set A pointer to the sparse set that contains the desired data type.
 * @param[in] e The ID of the @p Entity who the data belongs to.
 */
void *sparse_set_get(SparseSet *set, Entity e);

/** @brief Retrieves the pointer to the dense array inside of a given set.
 * @param[in] set The set from which to retrieve the dense array pointer.
 */
void *sparse_set_get_dense(SparseSet *set);

/** @brief Inserts data inside of a given set, associating an @p Entity ID with it.
 * @param[in] set A pointer to the sparse set where the data must be inserted into.
 * @param[in] e The ID of the @p Entity to which the data belongs to.
 * @retval true If the insertion was successful.
 * @retval false If the insertion failed.
 */
bool sparse_set_insert(SparseSet *set, Entity e, void *data);

/** @brief Debug function that prints information about the sparse set to stdout.*/
void sparse_set_debug_show(const SparseSet *set);

#endif // SPARSESET_H
