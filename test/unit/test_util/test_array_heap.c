/**
 * @file test_heap.c Test heaps
 * @author Evgeny Anisimoff <evgeny.anisimoff@gmail.com>
 */

#include "array_heap.h"
#include "test_macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    array_heap_t *heap;
    int i;

    heap = array_heap_new(25);
    for (i = 0; i < 25; i++)
        array_heap_add(heap, i, (void *) (long) i);
    for (i = 0; i < 25; i++) {
        void *p = array_heap_pop(heap);
        printf("--%d\n", (long) p);
        TEST_EQUAL((void *) (long)i, p);
        TEST_EQUAL(heap->size, 25 - i - 1);
    }
    return 0;
}
