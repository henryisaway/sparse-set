#include "ecs.h"

typedef struct {
    u32 x;
    u32 y;
    u32 z;
} Position;

typedef struct {
    u32 x;
    u32 y;
    u32 z;
} Velocity;

typedef u32 Health;

int main(void) {
    ecs_init();

    entity_reset_ids();

    register_component(Position);
    register_component(Velocity);
    register_component(Health);

    ecs_debug_show();

    ecs_shutdown();
    return 0;
}