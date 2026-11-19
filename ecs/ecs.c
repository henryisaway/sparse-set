#include "ecs.h"
#include <stdio.h>
#include <stdlib.h>
#include "../darray.h" // Will be used in the future to allow for multiple sparse sets.
#include "../sparseset.h"
#include "../type_aliases.h"

#define ECS_INIT_TYPE_COUNT 16

struct ECSContext {
    Entity    next_entity_id;
    TypeID    next_type_id;
    u64       capacity;
    u64      *type_strides;
    SparseSet *set;
};

static ECSContext *active_ctx = NULL;

void ecs_init() {
    ECSContext *new_ctx = malloc(sizeof(ECSContext));
    if (!new_ctx) exit(1);

    new_ctx->next_entity_id = 0;
    new_ctx->next_type_id   = 0;
    new_ctx->capacity       = ECS_INIT_TYPE_COUNT;
    new_ctx->type_strides   = malloc(ECS_INIT_TYPE_COUNT * sizeof(u64));

    active_ctx = new_ctx;
}

void ecs_shutdown() {
    if (active_ctx) free(active_ctx);
    active_ctx = NULL;
}

Entity entity_create(void) {
    ECSContext *ctx = active_ctx;
    if (!active_ctx) exit(1);

    return ctx->next_entity_id++;
}

void entity_reset_counter(void) {
    ECSContext *ctx = active_ctx;
    if (!active_ctx) exit(1);

    ctx->next_entity_id = 0;
}

TypeID ecs_register_component(u64 stride) {
    ECSContext *ctx = active_ctx;

    if (!ctx) exit(1);

    if (ctx->next_type_id >= ctx->capacity) {
        u64 new_capacity = 2 * ctx->capacity;
        u64 *new_array   = realloc(ctx->type_strides, new_capacity * sizeof(u64));
        if (!new_array) exit(1);

        ctx->capacity = new_capacity;
    }

    ctx->type_strides[ctx->next_type_id] = stride;
    ctx->set = sparse_set_create(stride);

    return ctx->next_type_id++;
}

void ecs_debug_show() {
    ECSContext *ctx = active_ctx;

    printf("ECS Context =================================\n");
    printf(".next_type_id = %lu\n", ctx->next_type_id);
    printf(".capacity = %lu\n", ctx->capacity);
    printf(".type_strides = [");

    for (u32 i = 0; i < ctx->next_type_id; i++) {
        if (i != 0) printf(", ");
        printf("%lu", ctx->type_strides[i]);
    }

    printf("]\n");
    printf(".set = ");
    sparse_set_debug_show(ctx->set);
    printf("END =========================================\n");
}