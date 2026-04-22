#include "maylloc.h"

#include <stdbool.h>
#include <stdlib.h>

struct maylloc_id {
    void* ptr;
    size_t size;
    bool in_use;
    maylloc_id_t* free_next;
    maylloc_id_t* all_next;
};

static maylloc_id_t* g_free_head;
static maylloc_id_t* g_all_head;

void maylloc_init(void)
{
    g_free_head = NULL;
    g_all_head = NULL;
}

void maylloc_deinit(void)
{
    maylloc_id_t* slot = g_all_head;
    while (slot) {
        maylloc_id_t* next = slot->all_next;
        if (slot->in_use)
            free(slot->ptr);
        free(slot);
        slot = next;
    }
    g_free_head = NULL;
    g_all_head = NULL;
}

maylloc_id_t* maylloc_alloc(size_t size)
{
    maylloc_id_t* slot;

    if (g_free_head) {
        slot = g_free_head;
        g_free_head = slot->free_next;
        slot->free_next = NULL;
    } else {
        slot = calloc(1, sizeof(maylloc_id_t));
        if (!slot)
            return NULL;
        slot->all_next = g_all_head;
        g_all_head = slot;
    }

    slot->ptr = malloc(size);
    if (!slot->ptr) {
        slot->free_next = g_free_head;
        g_free_head = slot;
        return NULL;
    }

    slot->size = size;
    slot->in_use = true;
    return slot;
}

void* maylloc_get(maylloc_id_t* id)
{
    if (!id || !id->in_use)
        return NULL;
    return id->ptr;
}

void maylloc_free(maylloc_id_t* id)
{
    if (!id || !id->in_use)
        return;

    free(id->ptr);
    id->ptr = NULL;
    id->size = 0;
    id->in_use = false;
    id->free_next = g_free_head;
    g_free_head = id;
}
