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

#ifndef _LIBUTIL_ARRAY_HEAP_H_
#define _LIBUTIL_ARRAY_HEAP_H_

#include <stdio.h>

#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/prim_type.h>
#include <sphinxbase/ckd_alloc.h>

  /** \file array_heap.h
   * \brief Heap Implementation based on array
   *
   * General Comment: Sorted heap structure with four main operations:
   *
   *   1. Insert a data item (with two attributes: an application supplied pointer and an
   *      integer value; the heap is maintained in ascending order of the integer value).
   *   2. Return the currently topmost item (i.e., item with smallest associated value).
   *   3. Return the currently topmost item and pop it off the heap.
   *   4. Iterate over all items (with no order guarantie)
   *
   */

#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

typedef struct array_heap_node_s {
    int32 key;
    void *value;
} array_heap_node_t;

typedef struct array_heap_s {
    uint32 size;
    uint32 capacity;
    array_heap_node_t *nodes;
} array_heap_t;

SPHINXBASE_EXPORT
array_heap_t *array_heap_new(uint32 capacity);

SPHINXBASE_EXPORT
void array_heap_free(array_heap_t *heap);

SPHINXBASE_EXPORT
int array_heap_full(array_heap_t *heap);

SPHINXBASE_EXPORT
void array_heap_add(array_heap_t *heap, int32 key, void *value);

SPHINXBASE_EXPORT
void *array_heap_element(array_heap_t *heap, uint32 index);

SPHINXBASE_EXPORT
int32 array_heap_min_key(array_heap_t *heap);

SPHINXBASE_EXPORT
void *array_heap_pop(array_heap_t *heap);

#ifdef __cplusplus
}
#endif

#endif /* _LIBUTIL_ARRAY_HEAP_H_ */
