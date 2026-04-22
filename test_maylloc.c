#include "maylloc.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int x;
    int y;
} Point;
typedef struct {
    char buf[128];
} BigThing;

static void test_basic(void)
{
    maylloc_id_t arena = mayllocInit(4096);
    assert(arena != MAYLLOC_NULL_ID);

    Point* p = MAYLLOC(arena, Point, 1);
    assert(p != NULL);
    p->x = 10;
    p->y = 20;
    assert(p->x == 10 && p->y == 20);

    mayllocDrop(arena);
    printf("PASS: test_basic\n");
}

static void test_multiple_types(void)
{
    maylloc_id_t arena = mayllocInit(4096);

    int* ints = MAYLLOC(arena, int, 16);
    Point* points = MAYLLOC(arena, Point, 8);
    char* buf = MAYLLOC(arena, char, 256);

    assert(ints && points && buf);

    /* pointers must not alias */
    assert((void*)ints != (void*)points);
    assert((void*)points != (void*)buf);

    for (int i = 0; i < 16; i++)
        ints[i] = i;
    for (int i = 0; i < 8; i++) {
        points[i].x = i;
        points[i].y = -i;
    }
    memset(buf, 0xAB, 256);

    for (int i = 0; i < 16; i++)
        assert(ints[i] == i);
    for (int i = 0; i < 8; i++) {
        assert(points[i].x == i && points[i].y == -i);
    }
    for (int i = 0; i < 256; i++)
        assert((unsigned char)buf[i] == 0xAB);

    mayllocDrop(arena);
    printf("PASS: test_multiple_types\n");
}

static void test_reset_reuses_memory(void)
{
    maylloc_id_t arena = mayllocInit(4096);

    int* first = MAYLLOC(arena, int, 4);
    assert(first != NULL);

    mayllocReset(arena);

    int* second = MAYLLOC(arena, int, 4);
    assert(second != NULL);

    /* After reset the bump pointer is back at 0, so same address reused. */
    assert(first == second);

    mayllocDrop(arena);
    printf("PASS: test_reset_reuses_memory\n");
}

static void test_null_id(void)
{
    assert(maylloc(MAYLLOC_NULL_ID, sizeof(int), 1) == NULL);
    mayllocReset(MAYLLOC_NULL_ID); /* must not crash */
    mayllocDrop(MAYLLOC_NULL_ID); /* must not crash */
    printf("PASS: test_null_id\n");
}

static void test_capacity_exhausted(void)
{
    /* Small arena — 64 bytes of data capacity. */
    maylloc_id_t arena = mayllocInit(64);
    assert(arena != MAYLLOC_NULL_ID);

    void* p = maylloc(arena, 1, 64);
    assert(p != NULL);

    /* One more byte must fail. */
    void* overflow = maylloc(arena, 1, 1);
    assert(overflow == NULL);

    mayllocDrop(arena);
    printf("PASS: test_capacity_exhausted\n");
}

static void test_overflow_protection(void)
{
    maylloc_id_t arena = mayllocInit(1024 * 1024);

    /* elem_size * count overflows size_t. */
    void* p = maylloc(arena, sizeof(uint64_t), (size_t)-1);
    assert(p == NULL);

    mayllocDrop(arena);
    printf("PASS: test_overflow_protection\n");
}

static void test_large_hint(void)
{
    /* 256 MiB hint — OS commits pages lazily so this is cheap. */
    maylloc_id_t arena = mayllocInit(256 * 1024 * 1024);
    assert(arena != MAYLLOC_NULL_ID);

    BigThing* arr = MAYLLOC(arena, BigThing, 1000);
    assert(arr != NULL);
    memset(arr, 0, sizeof(BigThing) * 1000);

    mayllocDrop(arena);
    printf("PASS: test_large_hint\n");
}

static void test_once(void)
{
    maylloc_id_t arena = mayllocInit(4096);

    Point* p = MAYLLOCONCE(arena, Point);
    assert(p != NULL);
    p->x = 7;
    p->y = 42;
    assert(p->x == 7 && p->y == 42);

    mayllocDrop(arena);
    printf("PASS: test_once\n");
}

static void test_zero_init(void)
{
    maylloc_id_t arena = mayllocInit(4096);

    /* Dirty the memory with a non-zero pattern, then reset. */
    int* dirty = MAYLLOC(arena, int, 8);
    assert(dirty != NULL);
    for (int i = 0; i < 8; i++)
        dirty[i] = 0xDEAD;
    mayllocReset(arena);

    /* Z variants must return zeroed memory even after reset. */
    int* clean = MAYLLOCZ(arena, int, 8);
    assert(clean != NULL);
    for (int i = 0; i < 8; i++)
        assert(clean[i] == 0);

    mayllocReset(arena);

    Point* p = MAYLLOCONCEZ(arena, Point);
    assert(p != NULL);
    assert(p->x == 0 && p->y == 0);

    mayllocDrop(arena);
    printf("PASS: test_zero_init\n");
}

static void test_stress(void)
{
    maylloc_id_t arena = mayllocInit(1024 * 1024);

    for (int round = 0; round < 10; round++) {
        int* nums = MAYLLOC(arena, int, 1000);
        assert(nums != NULL);
        for (int i = 0; i < 1000; i++)
            nums[i] = i * round;
        for (int i = 0; i < 1000; i++)
            assert(nums[i] == i * round);
        mayllocReset(arena);
    }

    mayllocDrop(arena);
    printf("PASS: test_stress\n");
}

static void test_dynamic_array_basic(void)
{
    maylloc_id_t arena = mayllocInit(64 * 1024);

    ArrayListint al = $initArrayList(arena, int);
    assert(al.items != NULL && al.len == 0 && al.cap == 4);

    for (int i = 0; i < 8; i++)
        $append(arena, &al, i * 10);

    assert(al.len == 8);
    for (int i = 0; i < 8; i++)
        assert(al.items[i] == i * 10);

    mayllocDrop(arena);
    printf("PASS: test_dynamic_array_basic\n");
}

static void test_dynamic_array_growth(void)
{
    maylloc_id_t arena = mayllocInit(1024 * 1024);

    ArrayListint al = $initArrayList(arena, int);

    /* Append enough to trigger multiple doublings (4 → 8 → 16 → ...). */
    for (int i = 0; i < 1000; i++)
        $append(arena, &al, i);

    assert(al.len == 1000);
    assert(al.cap >= 1000);
    for (int i = 0; i < 1000; i++)
        assert(al.items[i] == i);

    mayllocDrop(arena);
    printf("PASS: test_dynamic_array_growth\n");
}

static void test_dynamic_array_append_many(void)
{
    maylloc_id_t arena = mayllocInit(64 * 1024);

    ArrayListint dst = $initArrayList(arena, int);
    int buf[] = { 10, 20, 30, 40, 50 };
    Arrayint src = { .items = buf, .len = 5 };

    $appendMany(arena, &dst, src);

    assert(dst.len == 5);
    for (int i = 0; i < 5; i++)
        assert(dst.items[i] == buf[i]);

    mayllocDrop(arena);
    printf("PASS: test_dynamic_array_append_many\n");
}

static void test_dynamic_array_reverse(void)
{
    maylloc_id_t arena = mayllocInit(64 * 1024);

    ArrayListint al = $initArrayList(arena, int);
    for (int i = 0; i < 5; i++)
        $append(arena, &al, i + 1); /* 1 2 3 4 5 */

    $reverseArrayList(arena, &al); /* 5 4 3 2 1 */

    assert(al.len == 5);
    for (int i = 0; i < 5; i++)
        assert(al.items[i] == 5 - i);

    mayllocDrop(arena);
    printf("PASS: test_dynamic_array_reverse\n");
}

int main(void)
{
    test_basic();
    test_multiple_types();
    test_reset_reuses_memory();
    test_null_id();
    test_capacity_exhausted();
    test_overflow_protection();
    test_large_hint();
    test_once();
    test_zero_init();
    test_stress();
    test_dynamic_array_basic();
    test_dynamic_array_growth();
    test_dynamic_array_append_many();
    test_dynamic_array_reverse();

    printf("\nAll tests passed!\n");
    return 0;
}
