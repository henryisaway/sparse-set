#ifndef ECS_H_
#define ECS_H_

#include <stdint.h>
#include <stdbool.h>
#include "../type_aliases.h"

typedef void (*System)(void*);
typedef void (*System2)(void*, void*);
typedef struct ECSContext ECSContext;

/** @brief Registers a new component in the active ECS context.
 * @param[in] T The name of the component type.
 * @warning T @b must be a complete and valid type.
 */
#define register_component(T) \
    ecs_register_component(#T, sizeof(T));

/** @brief Initialises the ECS context. */
void ecs_init(void);

/** @brief Shuts down the ECS context. */
void ecs_shutdown(void);

/** @brief Generates a new @p Entity ID. */
Entity entity_create(void);

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