#include "ecs.h"

typedef struct {
    u32 x;
    u32 y;
    u32 z;
} Position;

int main(void) {
    ecs_init();

    register_component(Position);

    ecs_debug_show();

    ecs_shutdown();
    return 0;
}