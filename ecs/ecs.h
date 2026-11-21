#ifndef ECS_H_
#define ECS_H_

#include <stdint.h>
#include <stdbool.h>
#include "../sparseset.h"
#include "../darray.h"
#include "../type_aliases.h"

typedef void (*System)(void*);
typedef void (*System2)(void*, void*);
typedef struct ECSContext {
    Entity            next_entity_id;
    TypeID            next_type_id;
    u64               capacity;
    u64              *type_strides;
    DARRAY(SparseSet) sets;
} ECSContext;

/** @brief Registers a new component in the active ECS context.
 * @param[in] T The name of the component type.
 * @warning T @b must be a complete and valid type.
 */
#define register_component(T) \
    ecs_register_component(#T, sizeof(T));

#define _ecs_for_each(t, sys) do {\
    ECSContext *__ctx = ecs_get_context();\
    if (__ctx && t < __ctx->next_type_id) {\
        u32 __size       = __ctx->sets[t].size;\
        u64 __stride     = __ctx->type_strides[t];\
        void   *__data   = ecs_get_data(t);\
        if (__data) {\
            for (Entity __e = 0; __e < __size; __e++) {\
                void *__data_addr = (u8 *)(__data) + (__e * __stride);\
                sys(__data_addr);\
            }\
        }\
    }\
} while(0)

#define _ecs_for_each2(__t1, __t2, __sys) do {    \
    ECSContext *__ctx = ecs_get_context();  \
    if (__ctx && __t1 < __ctx->next_type_id && __t2 < __ctx->next_type_id) {                            \
        u32 __size1 = __ctx->sets[__t1].size;\
        u32 __size2 = __ctx->sets[__t2].size;\
        if (__size1 < __size2) {\
            u64 __stride1 = __ctx->type_strides[__t1];\
            void *__data1 = ecs_get_data(__t1);\
            if (__data1) {\
                Entity *__owners = ecs_get_owners(__t1);\
                SparseSet* __set2 = ecs_get_set(__t2);\
                for (Entity __e = 0; __e < __size1; __e++) {\
                    void *data1_addr = (u8 *)(__data1) + (__e * __stride1);\
                    Entity __owner = __owners[__e];\
                    void* __data2 = sparse_set_get(__set2, __owner);\
                    __sys(data1_addr, __data2);\
                }\
            }\
        } else {\
            u64 __stride2 = __ctx->type_strides[__t2];\
            void *__data2 = ecs_get_data(__t2);\
            if (__data2) {\
                Entity *__owners = ecs_get_owners(__t2);\
                SparseSet* __set1 = ecs_get_set(__t1);\
                for (Entity __e = 0; __e < __size2; __e++) {\
                    void *__data2_addr = (u8 *)(__data2) + (__e * __stride2);\
                    Entity __owner = __owners[__e];\
                    void* __data1 = sparse_set_get(__set1, __owner);\
                    __sys(__data1, __data2_addr);\
                }\
            }\
        }\
    }\
} while(0)

/** @brief Initialises the ECS context. */
void ecs_init(void);

/** @brief Shuts down the ECS context. */
void ecs_shutdown(void);

/** @brief Generates a new @p Entity ID. */
Entity entity_create(void);

/** @brief Returns a pointer to the currently active ECS context. */
ECSContext *ecs_get_context(void);

/** @brief Resets the entity ID generator back to 0. */
void entity_reset_ids(void);

/** @brief Binds a component to an entity
 * @param[in] t The @p TypeID of the component.
 * @param[in] e The @p Entity to which the component should be bound to.
 * @param[in] data A raw pointer containing the data to be bound to the @p 
 * Entity.
 * @retval true If the binding was successful.
 * @retval false If the binding failed.
 */
bool entity_bind_component(TypeID t, Entity e, void *data);

SparseSet *ecs_get_set(TypeID t);

/** @brief Returns the component data for a given component type and entity.
 * @param[in] t The @p TypeID of the component to be retrieved.
 * @param[in] e The @p Entity to which the component belongs.
 * @return A raw pointer to the component data.
 * @retval NULL if the component could not be found.
 */
void *ecs_get_component(TypeID t, Entity e);

/** @brief Retrieves a raw pointer to the array of component data of a given 
 * type.
 * @param[in] t The component type to which the data belongs to.
 * @return A raw pointer to the array of component data of the informed type.
 * @retval NULL if @t is an invalid @p TypeID.
 */
void *ecs_get_data(TypeID t);

/** @brief Retrieves a raw pointer to the array of owners of each component of 
 * a given type.
 * @param[in] t The component type to which the data belongs to.
 * @return A raw pointer to the array of component owners of the informed type.
 * @retval NULL if @t is an invalid @p TypeID.
 */
void *ecs_get_owners(TypeID t);

/** @brief Registers a component type into the ECS.
 * @param stride The size of the component in bytes.
 * @return An unsigned 64-bit ID that identifies that component type.
 * 
 * @warning It can currently only register a single component type.
 */
TypeID ecs_register_component(const char *name, u64 stride);

/** @brief Applies a system to a component type.
 * @details For each entity that owns a component of type @p t, this 
 * function applies the user-defined callback function @p sys. The callback 
 * function @b must have a `void function(void*)` signature.
 * 
 * @param[in] t The @p TypeID of the component to operate on.
 * @param[in] sys A pointer to the callback function that operates on the 
 * components of type @p t.
 */
void ecs_for_each(TypeID t, System sys);

/** @brief Applies a system to two component types.
 * @details For each entity that owns components of types @p t1 and @p t2, this 
 * function applies the user-defined callback function @p sys. The callback 
 * function @b must have a `void function(void*, void*)` signature.
 * 
 * It is guaranteed that the first argument provided to the @p sys callback 
 * will be of type @p t1, and the second argument provided will be of type 
 * @p t2.
 * 
 * @param[in] t1 The @p TypeID of the first component.
 * @param[in] t2 The @p TypeID of the second component.
 * @param[in] sys A pointer to the callback function that operates on the two 
 * components @p t1 and @p t2.
 */
void ecs_for_each2(TypeID t1, TypeID t2, System2 sys);

/** @brief Debugging function that prints all values held by the ECS context. 
 */
void ecs_debug_show(void);

#endif