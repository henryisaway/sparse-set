#include "ecs.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#define MIRA_DARRAY_IMPL
#include "../darray.h"
#include "../sparseset.h"
#include "../type_aliases.h"

#define ECS_INIT_TYPE_COUNT 2

struct ECSContext {
    Entity            next_entity_id;
    TypeID            next_type_id;
    u64               capacity;
    u64              *type_strides;
    DARRAY(SparseSet) sets;
};

static ECSContext *active_ctx = NULL;

void ecs_init(void) {
    ECSContext *new_ctx = malloc(sizeof(ECSContext));
    if (!new_ctx) exit(1);

    new_ctx->next_entity_id = 0;
    new_ctx->next_type_id   = 0;
    new_ctx->capacity       = ECS_INIT_TYPE_COUNT;
    new_ctx->type_strides   = malloc(ECS_INIT_TYPE_COUNT * sizeof(u64));

    active_ctx = new_ctx;
}

void ecs_shutdown(void) {
    if (active_ctx) {
        if (active_ctx->sets) {
            DARRAY_FOREACH(SparseSet, set, active_ctx->sets) {
                sparse_set_destroy(set);
            }
            DARRAY_FREE(active_ctx->sets);
        }
        free(active_ctx->type_strides);
        free(active_ctx);
    }
    active_ctx = NULL;
}

Entity entity_create(void) {
    ECSContext *ctx = active_ctx;
    if (!active_ctx) exit(1);

    return ctx->next_entity_id++;
}

void entity_reset_ids(void) {
    ECSContext *ctx = active_ctx;
    if (!active_ctx) exit(1);

    ctx->next_entity_id = 0;
}

bool entity_bind_component(TypeID t, Entity e, void *data) {
    ECSContext *ctx = active_ctx;
    if (!ctx) exit(1);

    if (t >= ctx->next_type_id) exit(1);
    SparseSet *set = &ctx->sets[t];

    return sparse_set_insert(set, e, data);
}

void *ecs_get_component(TypeID t, Entity e) {
    ECSContext *ctx = active_ctx;
    if (!ctx) exit(1);

    if (t >= ctx->next_type_id) exit(1);
    SparseSet *set = &ctx->sets[t];

    return sparse_set_get(set, e);
}

void *ecs_get_data(TypeID t) {
    ECSContext *ctx = active_ctx;
    if (!ctx) exit(1);

    if (t >= ctx->next_type_id) return NULL;
    
    return sparse_set_get_data(&ctx->sets[t]);
}

void *ecs_get_owners(TypeID t) {
    ECSContext *ctx = active_ctx;
    if (!ctx) exit(1);

    if (t >= ctx->next_type_id) return NULL;
    
    return sparse_set_get_owners(&ctx->sets[t]);
}

TypeID ecs_register_component(const char *name, u64 stride) {
    ECSContext *ctx = active_ctx;
    if (!ctx) exit(1);

    if (ctx->next_type_id >= ctx->capacity) {
        u64 new_capacity = 2 * ctx->capacity;

        u64 *new_array   = realloc(ctx->type_strides, new_capacity * sizeof(u64));
        if (!new_array) exit(1);
        ctx->type_strides = new_array;

        ctx->capacity = new_capacity;
    }

    ctx->type_strides[ctx->next_type_id] = stride;
    DARRAY_PUSH(ctx->sets, sparse_set_create(name, stride));

    return ctx->next_type_id++;
}

void ecs_for_each(TypeID t, System sys) {
    ECSContext *ctx = active_ctx;
    if (!ctx) exit(1);

    // Type doesn't exist
    if (t >= ctx->next_type_id) exit(1);

    u32 size       = ctx->sets[t].size;
    u64 stride     = ctx->type_strides[t];
    void   *data   = ecs_get_data(t);
    if (!data) exit(1);

    for (u64 i = 0; i < size; i++) {
        void *data_addr = (u8 *)(data) + (i * stride);

        sys(data_addr);
    }
}

void ecs_for_each2(TypeID t1, TypeID t2, System2 sys) {
    ECSContext *ctx = active_ctx;
    if (!ctx) exit(1);

    // Check if both types exist
    if (t1 >= ctx->next_type_id) exit(1);
    if (t2 >= ctx->next_type_id) exit(1);

    u32 size1 = ctx->sets[t1].size;
    u32 size2 = ctx->sets[t2].size;

    // Iterate over the smallest of the two sets
    if (size1 < size2) {
        u64 stride1 = ctx->type_strides[t1];
        void *data1 = ecs_get_data(t1);
        Entity *owners = ecs_get_owners(t1);

        for (u64 i = 0; i < size1; i++) {
            void *data1_addr = (u8 *)(data1) + (i * stride1);
            Entity owner = owners[i];

            void* data2 = ecs_get_component(t2, owner);

            sys(data1_addr, data2);
        }
    } else {
        u64 stride2 = ctx->type_strides[t2];
        void *data2 = ecs_get_data(t2);
        Entity *owners = ecs_get_owners(t2);

        for (u64 i = 0; i < size2; i++) {
            void *data2_addr = (u8 *)(data2) + (i * stride2);
            Entity owner = owners[i];

            void* data1 = ecs_get_component(t1, owner);

            sys(data1, data2_addr);
        }
    }
}

void ecs_debug_show(void) {
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
    printf(".set = [");

    DARRAY_FOREACH(SparseSet, set, ctx->sets) {
        if (set != ctx->sets) printf(",\n");
        sparse_set_debug_show(set);
    }

    printf("]");
    printf("END =========================================\n");
}