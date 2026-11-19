#include "../sparseset.h"

#include <stdint.h>

typedef uint64_t TypeID;
typedef struct ECSContext ECSContext;

#define register_component(T) \
    ecs_register_component(sizeof(T));

/** @brief Initialises the ECS context. */
void ecs_init();

/** @brief Shuts down the ECS context. */
void ecs_shutdown();

/** @brief Registers a component type into the ECS.
 * @param stride The size of the component in bytes.
 * @return An unsigned 64-bit ID that identifies that component type.
 */
TypeID ecs_register_component(u64 stride);
