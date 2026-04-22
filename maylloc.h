#pragma once

#include <stddef.h>

typedef size_t maylloc_id_t;

#define MAYLLOC_NULL_ID ((maylloc_id_t)0)

/* Create a new arena. size_hint is a rough guide for the reserved capacity;
   the OS commits pages lazily so large values are cheap to request. */
maylloc_id_t mayllocInit(size_t size_hint);

/* Allocate count elements of elem_size bytes from the arena.
   Returns NULL on failure (out of capacity or arithmetic overflow). */
void* maylloc(maylloc_id_t id, size_t elem_size, size_t count);

/* Typed allocation shorthand: MAYLLOC(arena, MyStruct, 8) -> MyStruct* */
#define MAYLLOC(id, Type, count) ((Type*)maylloc((id), sizeof(Type), (count)))

/* Allocate a single element of elem_size bytes. */
void* mayllocOnce(maylloc_id_t id, size_t elem_size);

/* Typed single-element shorthand: MAYLLOCONCE(arena, MyStruct) -> MyStruct* */
#define MAYLLOCONCE(id, Type) ((Type*)mayllocOnce((id), sizeof(Type)))

/* Zero-initializing variants — memory is explicitly zeroed before returning.
   Useful after mayllocReset when arenas are reused across cycles. */
void* mayllocZ(maylloc_id_t id, size_t elem_size, size_t count);
void* mayllocOnceZ(maylloc_id_t id, size_t elem_size);

#define MAYLLOCZ(id, Type, count) ((Type*)mayllocZ((id), sizeof(Type), (count)))
#define MAYLLOCONCEZ(id, Type) ((Type*)mayllocOnceZ((id), sizeof(Type)))

/* Invalidate all previous pointers and reset the arena for reuse.
   No memory is released — subsequent allocations reuse the same pages. */
void mayllocReset(maylloc_id_t id);

/* Release the arena and all its memory. */
void mayllocDrop(maylloc_id_t id);
