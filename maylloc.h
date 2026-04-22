#ifndef MAYLLOC_H
#define MAYLLOC_H

#include <stddef.h>

typedef struct maylloc_id maylloc_id_t;

void maylloc_init(void);
void maylloc_deinit(void);
maylloc_id_t* maylloc_alloc(size_t size);
void* maylloc_get(maylloc_id_t* id);
void maylloc_free(maylloc_id_t* id);

#endif
