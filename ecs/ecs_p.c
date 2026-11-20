#include <stdio.h>
#include "ecs.h"

typedef struct {
    u32 x;
    u32 y;
} Position;

typedef struct {
    u32 x;
    u32 y;
} Velocity;

typedef u32 Health;

int main(void) {
    ecs_init();

    entity_reset_ids();

    TypeID pos_ID = register_component(Position);
    TypeID vel_ID = register_component(Velocity);
    TypeID hp_ID  = register_component(Health);

    Entity player = entity_create();

    entity_bind_component(pos_ID, player, &(Position){.x = 5, .y = 2});
    entity_bind_component(vel_ID, player, &(Velocity){.x = 1, .y = 2});
    entity_bind_component(hp_ID, player, &(u32){100});

    Position *pos = (Position *) ecs_get_component(pos_ID, player);
    printf("The position of Entity %lu is {%u, %u}\n", player, pos->x, pos->y);

    Velocity *vel = (Velocity *) ecs_get_component(vel_ID, player);
    printf("The velocity of Entity %lu is {%u, %u}\n", player, vel->x, vel->y);

    Health *hp = (Health *) ecs_get_component(hp_ID, player);
    printf("The health of Entity %lu is %u\n", player, *hp);

    ecs_debug_show();

    ecs_shutdown();
    return 0;
}