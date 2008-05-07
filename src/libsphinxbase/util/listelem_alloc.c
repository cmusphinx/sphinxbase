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

#include <stdio.h>
#include <stdlib.h>

#include <err.h>
#include <ckd_alloc.h>
#include <listelem_alloc.h>
#include <glist.h>

/**
 * Fast linked list allocator.
 * 
 * We keep a separate linked list for each element-size.  Element-size
 * must be a multiple of pointer-size.
 *
 * Initially a block of empty elements is allocated, where the first
 * machine word in each element points to the next available element.
 * To allocate, we use this pointer to move the freelist to the next
 * element, then return the current element.
 *
 * The last element in the list starts with a NULL pointer, which is
 * used as a signal to allocate a new block of elements.
 *
 * In order to be able to actually release the memory allocated, we
 * have to add a linked list of block pointers.  This shouldn't create
 * much overhead since we never access it except when freeing the
 * allocator.
 */
struct listelem_alloc_s {
    char **freelist;            /**< ptr to first element in freelist */
    glist_t blocks;             /**< Linked list of blocks allocated. */
    size_t elemsize;            /**< Number of (char *) in element */
    size_t blocksize;           /**< Number of elements to alloc if run out of free elments */
    size_t blk_alloc;           /**< Number of alloc operations before increasing blocksize */
    size_t n_alloc;
    size_t n_freed;
};

#define MIN_ALLOC	50      /**< Minimum number of elements to allocate in one block */
/**
 * Allocate a new block of elements.
 */
static void listelem_add_block(listelem_alloc_t *list,
			       char *caller_file, int caller_line);

listelem_alloc_t *
listelem_alloc_init(size_t elemsize)
{
    listelem_alloc_t *list;

    if ((elemsize % sizeof(void *)) != 0) {
        size_t rounded = (elemsize + sizeof(void *) - 1) & ~(sizeof(void *)-1);
        E_WARN
            ("List item size (%lu) not multiple of sizeof(void *), rounding to %lu\n",
             (unsigned long)elemsize,
             (unsigned long)rounded);
        elemsize = rounded;
    }
    list = ckd_calloc(1, sizeof(*list));
    list->freelist = NULL;
    list->blocks = NULL;
    list->elemsize = elemsize;
    list->blocksize = MIN_ALLOC;
    list->blk_alloc = (1 << 18) / (list->blocksize * sizeof(elemsize));
    list->n_alloc = 0;
    list->n_freed = 0;

    /* Allocate an initial block to minimize latency. */
    listelem_add_block(list, __FILE__, __LINE__);
    return list;
}

void
listelem_alloc_free(listelem_alloc_t *list)
{
    gnode_t *gn;
    if (list == NULL)
	return;
    for (gn = list->blocks; gn; gn = gnode_next(gn))
	ckd_free(gnode_ptr(gn));
    glist_free(list->blocks);
    ckd_free(list);
}

static void
listelem_add_block(listelem_alloc_t *list, char *caller_file, int caller_line)
{
    char **cpp, *cp;
    size_t j;

    /* Check if block size should be increased (if many requests for this size) */
    if (list->blk_alloc == 0) {
	list->blocksize <<= 1;
	list->blk_alloc =
	    (1 << 18) / (list->blocksize * sizeof(list->elemsize));
	if (list->blk_alloc <= 0)
	    list->blk_alloc = (size_t) 0x70000000;   /* Limit blocksize to new value */
    }

    /* Allocate block */
    cpp = list->freelist =
	(char **) __ckd_calloc__(list->blocksize, list->elemsize,
				 caller_file, caller_line);
    list->blocks = glist_add_ptr(list->blocks, cpp);
    cp = (char *) cpp;
    /* Link up the blocks via their first machine word. */
    for (j = list->blocksize - 1; j > 0; --j) {
	cp += list->elemsize;
	*cpp = cp;
	cpp = (char **) cp;
    }
    /* Make sure the last element's forward pointer is NULL */
    *cpp = NULL;
    --(list->blk_alloc);
}


void *
__listelem_malloc__(listelem_alloc_t *list, char *caller_file, int caller_line)
{
    void *ptr;

    /* Allocate a new block if list empty */
    if (list->freelist == NULL)
	listelem_add_block(list, caller_file, caller_line);

    /* Unlink and return first element in freelist */
    ptr = list->freelist;
    list->freelist = (char **) (*(list->freelist));
    (list->n_alloc)++;

    return ptr;
}


void
__listelem_free__(listelem_alloc_t *list, void *elem,
                  char *caller_file, int caller_line)
{
    char **cpp;

    /*
     * Insert freed item at head of list.
     */
    cpp = (char **) elem;
    *cpp = (char *) list->freelist;
    list->freelist = cpp;
    (list->n_freed)++;
}


void
listelem_stats(listelem_alloc_t *list)
{
    gnode_t *gn;
    char **cpp;
    size_t n;

    E_INFO("Linklist stats:\n");
    for (n = 0, cpp = list->freelist; cpp;
         cpp = (char **) (*cpp), n++);
    E_INFO
        ("elemsize %lu, blocksize %lu, #alloc %lu, #freed %lu, #freelist %lu\n",
         (unsigned long)list->elemsize,
         (unsigned long)list->blocksize,
         (unsigned long)list->n_alloc,
         (unsigned long)list->n_freed,
         (unsigned long)n);
    E_INFO("Allocated blocks:\n");
    for (gn = list->blocks; gn; gn = gnode_next(gn))
	E_INFO("%p\n", gnode_ptr(gn));
}
