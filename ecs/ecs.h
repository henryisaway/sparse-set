#ifndef ECS_H_
#define ECS_H_

#include <stdint.h>
#include "../type_aliases.h"

typedef struct ECSContext ECSContext;

#define register_component(T) \
    ecs_register_component(sizeof(T));

/** @brief Initialises the ECS context. */
void ecs_init();

/** @brief Shuts down the ECS context. */
void ecs_shutdown();

Entity     entity_create(void);
void       entity_reset_counter(void);

/** @brief Registers a component type into the ECS.
 * @param stride The size of the component in bytes.
 * @return An unsigned 64-bit ID that identifies that component type.
 * 
 * @warning It can currently only register a single component type.
 */
TypeID ecs_register_component(u64 stride);

/** @brief Debugging function that prints all values held by the ECS context. */
void ecs_debug_show();

#endif