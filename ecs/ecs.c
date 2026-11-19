#include "ecs.h"
#include <stdlib.h>

#define ECS_INIT_TYPE_COUNT 16

struct ECSContext {
    TypeID  id_count;
    u64     capacity;
    u64     type_strides[];
};

static ECSContext *active_ctx = NULL;

void ecs_init() {
    ECSContext *new_ctx = malloc(sizeof(ECSContext) + (ECS_INIT_TYPE_COUNT * sizeof(u64)));
    if (!new_ctx) exit(1);

    new_ctx->id_count = 0;
    new_ctx->capacity = ECS_INIT_TYPE_COUNT;

    active_ctx = new_ctx;
}

void ecs_shutdown() {
    if (active_ctx) free(active_ctx);
    active_ctx = NULL;
}

TypeID ecs_register_component(u64 stride) {
    ECSContext *ctx = active_ctx;

    if (ctx->id_count >= ctx->capacity) {
        u64 new_capacity = 2 * ctx->capacity;
        u64 *new_array = realloc(ctx->type_strides, new_capacity * sizeof(u64));
        if (!new_array) exit(1);

        ctx->capacity = new_capacity;
    }

    ctx->type_strides[ctx->id_count] = stride;

    return ctx->id_count++;
}