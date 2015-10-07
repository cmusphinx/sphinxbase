/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2004 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced
 * Research Projects Agency and the National Science Foundation of the
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */
/*
 * heap.c -- Heap structure for inserting in any and popping in sorted
 *      order.
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1999 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 */

#include "sphinxbase/array_heap.h"

array_heap_t *array_heap_new(uint32 capacity) {
    array_heap_t *heap = ckd_malloc(sizeof(array_heap_t));
    heap->size = 0;
    heap->capacity = capacity;
    heap->nodes = ckd_calloc(capacity, sizeof(array_heap_node_t));
    return heap;
}

void array_heap_free(array_heap_t *heap) {
    ckd_free(heap->nodes);
    ckd_free(heap);
}

uint32 array_heap_parent(uint32 index) {
    return (index - 1) / 2;
}

uint32 array_heap_left_child(uint32 index) {
    return 2 * index + 1;
}

uint32 array_heap_right_child(uint32 index) {
    return 2 * index + 2;
}

int array_heap_full(array_heap_t *heap) {
    return heap->capacity <= heap->size;
}

void array_heap_add(array_heap_t *heap, int32 key, void *value) {
    uint32 current;

    if (array_heap_full(heap)) {
        E_ERROR("Trying to add element to a full heap");
        return;
    }

    current = heap->size;
    heap->size++;
    while (current != 0 && heap->nodes[array_heap_parent(current)].key > key) {
        heap->nodes[current] = heap->nodes[array_heap_parent(current)];
        current = array_heap_parent(current);
    }
    heap->nodes[current].key = key;
    heap->nodes[current].value = value;
}

void *array_heap_element(array_heap_t *heap, uint32 index) {
    return heap->nodes[index].value;
}

int32 array_heap_min_key(array_heap_t *heap) {
    if (heap->size == 0) {
        E_ERROR("Trying to get minimum element from empty heap");
    }
    return heap->nodes[0].key;
}

void *array_heap_pop(array_heap_t *heap) {
    void *result;
    int32 current, min;
    array_heap_node_t temp;

    if (heap->size == 0) {
        E_ERROR("Trying to get minimum element from empty heap");
    }
    result = heap->nodes[0].value;
    heap->nodes[0] = heap->nodes[heap->size - 1];
    heap->size--;
    current = 0;
    while (TRUE) {
        min = current;
        if (array_heap_left_child(current) < heap->size && heap->nodes[array_heap_left_child(current)].key
                < heap->nodes[min].key) {
            min = array_heap_left_child(current);
        }
        if (array_heap_right_child(current) < heap->size && heap->nodes[array_heap_right_child(current)].key
                < heap->nodes[min].key) {
            min = array_heap_right_child(current);
        }
        if (min == current) {
            return result;
        }
        temp = heap->nodes[min];
        heap->nodes[min] = heap->nodes[current];
        heap->nodes[current] = temp;
        current = min;
    }
}

