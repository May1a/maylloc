# maylloc

A minimal arena allocator for C. Each arena is a single `mmap` reservation — the OS commits pages lazily, so large upfront hints are cheap. All memory is freed with one `munmap` call via `mayllocDrop`.

## How it works

```
mmap reservation (size_hint)
┌─────────────┬────────────────────────────────────────┐
│ arena header│ ← bump pointer grows this way          │
└─────────────┴────────────────────────────────────────┘
               ↑ used = 0
```

`maylloc` advances the bump pointer. `mayllocReset` moves it back to zero without releasing the pages — subsequent allocations reuse the same physical memory.

## API

### Arena lifecycle

```c
// Reserve an arena. size_hint is a rough capacity guide.
maylloc_id_t mayllocInit(size_t size_hint);

// Reset bump pointer to zero. Reuses pages, invalidates all pointers.
void mayllocReset(maylloc_id_t id);

// Release the arena. Single munmap.
void mayllocDrop(maylloc_id_t id);
```

### Allocation

| Function | Macro | Description |
|---|---|---|
| `maylloc(id, size, count)` | `MAYLLOC(id, Type, n)` | Allocate `n` elements |
| `mayllocOnce(id, size)` | `MAYLLOCONCE(id, Type)` | Allocate one element |
| `mayllocZ(id, size, count)` | `MAYLLOCZ(id, Type, n)` | Allocate `n` elements, zeroed |
| `mayllocOnceZ(id, size)` | `MAYLLOCONCEZ(id, Type)` | Allocate one element, zeroed |

All functions return `NULL` on failure (capacity exhausted or arithmetic overflow). The typed macros are the recommended interface.

## Usage

```c
#include "maylloc.h"

typedef struct { float x, y, z; } Vec3;

// --- Basic ---
maylloc_id_t arena = mayllocInit(1024 * 1024); // 1 MiB hint

Vec3* verts = MAYLLOC(arena, Vec3, 256);
int*  ids   = MAYLLOC(arena, int,  256);

// Everything freed at once
mayllocDrop(arena);

// --- Per-frame scratch (zero syscalls after warmup) ---
maylloc_id_t scratch = mayllocInit(512 * 1024);

while (running) {
    Work* jobs = MAYLLOCZ(arena, Work, 64); // zeroed, reused each frame
    process(jobs);
    mayllocReset(scratch);
}

mayllocDrop(scratch);
```

## Build

```sh
make        # build test binary
make test   # build and run tests
make clean
```

Requires a C99 compiler and a POSIX system with `mmap`.
