#ifndef ECS_H_
#define ECS_H_

#include <stdint.h>
#include <stdbool.h>
#include "../type_aliases.h"

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
 * @param[in] data A raw pointer containing the data to be bound to the @p Entity.
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
void *entity_get_component(TypeID t, Entity e);

/** @brief Registers a component type into the ECS.
 * @param stride The size of the component in bytes.
 * @return An unsigned 64-bit ID that identifies that component type.
 * 
 * @warning It can currently only register a single component type.
 */
TypeID ecs_register_component(const char *name, u64 stride);

/** @brief Debugging function that prints all values held by the ECS context. */
void ecs_debug_show(void);

#endif