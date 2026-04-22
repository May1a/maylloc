#include "maylloc.h"

#include <sys/mman.h>

#define MAYLLOC_ALIGN       16
#define MAYLLOC_DEFAULT_CAP (64 * 1024)  /* 64 KiB */

typedef struct {
    size_t total;  /* full mmap size, stored for munmap */
    size_t used;   /* bytes consumed in the data region */
} maylloc_arena_t;

static size_t align_up(size_t n)
{
    return (n + MAYLLOC_ALIGN - 1) & ~(size_t)(MAYLLOC_ALIGN - 1);
}

/* Offset at which the data region begins within the mmap. */
static size_t header_size(void)
{
    return align_up(sizeof(maylloc_arena_t));
}

static char* arena_data(maylloc_arena_t* a)
{
    return (char*)a + header_size();
}

/* Usable capacity of the arena's data region. */
static size_t arena_cap(maylloc_arena_t* a)
{
    return a->total - header_size();
}

maylloc_id_t mayllocInit(size_t size_hint)
{
    if (size_hint == 0)
        size_hint = MAYLLOC_DEFAULT_CAP;

    size_t total = header_size() + align_up(size_hint);

    maylloc_arena_t* a = mmap(NULL, total,
                               PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (a == MAP_FAILED)
        return MAYLLOC_NULL_ID;

    a->total = total;
    a->used  = 0;
    return (maylloc_id_t)a;
}

void* maylloc(maylloc_id_t id, size_t elem_size, size_t count)
{
    if (id == MAYLLOC_NULL_ID || elem_size == 0 || count == 0)
        return NULL;

    /* Overflow guard for elem_size * count. */
    if (count > (size_t)-1 / elem_size)
        return NULL;
    size_t bytes  = elem_size * count;
    size_t needed = align_up(bytes);
    if (needed < bytes)  /* align_up itself overflowed */
        return NULL;

    maylloc_arena_t* a = (maylloc_arena_t*)id;

    /* arena_cap(a) >= a->used is maintained as an invariant. */
    if (needed > arena_cap(a) - a->used)
        return NULL;

    void* ptr = arena_data(a) + a->used;
    a->used += needed;
    return ptr;
}

void mayllocReset(maylloc_id_t id)
{
    if (id == MAYLLOC_NULL_ID)
        return;
    ((maylloc_arena_t*)id)->used = 0;
}

void mayllocDrop(maylloc_id_t id)
{
    if (id == MAYLLOC_NULL_ID)
        return;
    maylloc_arena_t* a = (maylloc_arena_t*)id;
    munmap(a, a->total);
}
