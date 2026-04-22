#include "maylloc.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_basic_alloc_free(void)
{
    maylloc_init();

    maylloc_id_t* id = maylloc_alloc(64);
    assert(id != NULL);

    void* ptr = maylloc_get(id);
    assert(ptr != NULL);

    memset(ptr, 0xAB, 64);

    maylloc_free(id);
    assert(maylloc_get(id) == NULL);

    maylloc_deinit();
    printf("PASS: test_basic_alloc_free\n");
}

static void test_double_free(void)
{
    maylloc_init();

    maylloc_id_t* id = maylloc_alloc(32);
    assert(id != NULL);

    maylloc_free(id);
    maylloc_free(id);

    maylloc_deinit();
    printf("PASS: test_double_free\n");
}

static void test_null_id(void)
{
    maylloc_init();

    assert(maylloc_get(NULL) == NULL);
    maylloc_free(NULL);

    maylloc_deinit();
    printf("PASS: test_null_id\n");
}

static void test_multiple_allocs(void)
{
    maylloc_init();

    maylloc_id_t* ids[100];
    for (int i = 0; i < 100; i++) {
        ids[i] = maylloc_alloc(64);
        assert(ids[i] != NULL);
    }

    for (int i = 0; i < 100; i++) {
        void* pi = maylloc_get(ids[i]);
        assert(pi != NULL);
        for (int j = i + 1; j < 100; j++) {
            assert(pi != maylloc_get(ids[j]));
        }
    }

    for (int i = 0; i < 100; i++)
        maylloc_free(ids[i]);

    maylloc_deinit();
    printf("PASS: test_multiple_allocs\n");
}

static void test_alloc_failure(void)
{
    maylloc_init();

    maylloc_id_t* id = maylloc_alloc((size_t)-1);
    assert(id == NULL);

    maylloc_deinit();
    printf("PASS: test_alloc_failure\n");
}

static void test_slot_reuse(void)
{
    maylloc_init();

    maylloc_id_t* id1 = maylloc_alloc(32);
    assert(id1 != NULL);
    maylloc_free(id1);

    maylloc_id_t* id2 = maylloc_alloc(32);
    assert(id2 != NULL);
    assert(id2 == id1);

    maylloc_free(id2);
    maylloc_deinit();
    printf("PASS: test_slot_reuse\n");
}

static void test_stress(void)
{
    maylloc_init();

    maylloc_id_t* ids[1000];
    for (int i = 0; i < 1000; i++) {
        ids[i] = maylloc_alloc(128);
        assert(ids[i] != NULL);
        memset(maylloc_get(ids[i]), i & 0xFF, 128);
    }

    for (int i = 0; i < 1000; i++)
        maylloc_free(ids[i]);

    maylloc_deinit();
    printf("PASS: test_stress\n");
}

int main(void)
{
    test_basic_alloc_free();
    test_double_free();
    test_null_id();
    test_multiple_allocs();
    test_alloc_failure();
    test_slot_reuse();
    test_stress();

    printf("\nAll tests passed!\n");
    return 0;
}
