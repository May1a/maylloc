#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

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

/* ---------------------------------------------------------------------------
 * Arena-backed dynamic arrays
 * Requires GNU extensions (typeof, statement expressions — gnu99 or later).
 *
 * Growth allocates a fresh buffer from the arena and copies; the old buffer
 * becomes unreachable dead space reclaimed on mayllocDrop / mayllocReset.
 * --------------------------------------------------------------------------- */

/* Define a typed ArrayList<T> and a non-owning Array<T> slice. */
#define $defArrayList(T) \
    typedef struct {     \
        T* items;        \
        size_t len;      \
        size_t cap;      \
    } ArrayList##T;      \
    typedef struct {     \
        T* items;        \
        size_t len;      \
    } Array##T

/* Create an ArrayList backed by arena with an initial capacity of 4. */
#define $initArrayList(arena, T)                \
    ({                                          \
        size_t _cap = 4;                        \
        (ArrayList##T) {                        \
            .items = MAYLLOC((arena), T, _cap), \
            .len = 0,                           \
            .cap = _cap,                        \
        };                                      \
    })

/* Append a single item, growing into the arena if needed. */
#define $append(arena, al, item)                                                            \
    ({                                                                                      \
        if ((al)->len >= (al)->cap) {                                                       \
            size_t _newCap = (al)->cap * 2;                                                 \
            typeof((al)->items[0])* _p = MAYLLOC((arena), typeof((al)->items[0]), _newCap); \
            memcpy(_p, (al)->items, (al)->len * sizeof((al)->items[0]));                    \
            (al)->items = _p;                                                               \
            (al)->cap = _newCap;                                                            \
        }                                                                                   \
        (al)->items[(al)->len++] = (item);                                                  \
    })

/* Append all elements from an Array slice. */
#define $appendMany(arena, al, slice)                                                       \
    ({                                                                                      \
        if ((al)->len + (slice).len > (al)->cap) {                                          \
            size_t _newCap = ((al)->len + (slice).len) * 2;                                 \
            typeof((al)->items[0])* _p = MAYLLOC((arena), typeof((al)->items[0]), _newCap); \
            memcpy(_p, (al)->items, (al)->len * sizeof((al)->items[0]));                    \
            (al)->items = _p;                                                               \
            (al)->cap = _newCap;                                                            \
        }                                                                                   \
        memcpy((al)->items + (al)->len, (slice).items,                                      \
            (slice).len * sizeof((al)->items[0]));                                          \
        (al)->len += (slice).len;                                                           \
    })

/* Reverse the array into a new arena allocation. */
#define $reverseArrayList(arena, al)                                                      \
    ({                                                                                    \
        typeof((al)->items[0])* _p = MAYLLOC((arena), typeof((al)->items[0]), (al)->len); \
        for (size_t _i = 0; _i < (al)->len; _i++)                                         \
            _p[_i] = (al)->items[(al)->len - 1 - _i];                                     \
        (al)->items = _p;                                                                 \
    })

$defArrayList(char);
$defArrayList(int);
$defArrayList(size_t);
$defArrayList(uint32_t);
$defArrayList(int32_t);
$defArrayList(uint16_t);
$defArrayList(int16_t);
$defArrayList(int8_t);
$defArrayList(uint8_t);
