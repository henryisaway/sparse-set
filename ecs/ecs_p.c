#include "ecs.h"

typedef struct {
    u32 x;
    u32 y;
} Position;

int main(void) {
    ecs_init();
    ecs_shutdown();
    return 0;
}