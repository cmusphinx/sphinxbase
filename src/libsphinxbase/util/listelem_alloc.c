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
    size_t n_blocks;            /**< Number of blocks allocated so far. */
    size_t n_alloc;
    size_t n_freed;
};

#define MIN_ALLOC	50      /**< Minimum number of elements to allocate in one block */
#define BLKID_SHIFT     16      /**< Bit position of block number in element ID */
#define BLKID_MASK ((1<<BLKID_SHIFT)-1)

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
    /* Intent of this is to increase block size once we allocate
     * 256KiB (i.e. 1<<18). If somehow the element size is big enough
     * to overflow that, just fail, people should use malloc anyway. */
    list->blk_alloc = (1 << 18) / (list->blocksize * elemsize);
    if (list->blk_alloc <= 0) {
        E_ERROR("Element size * block size exceeds 256k, use malloc instead.\n");
        ckd_free(list);
        return NULL;
    }
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
        /* See above.  No sense in allocating blocks bigger than
         * 256KiB (well, actually, there might be, but we'll worry
         * about that later). */
	list->blocksize <<= 1;
        if (list->blocksize * list->elemsize > (1 << 18))
            list->blocksize = (1 << 18) / list->elemsize;
	list->blk_alloc = (1 << 18) / (list->blocksize * list->elemsize);
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
    --list->blk_alloc;
    ++list->n_blocks;
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

void *
__listelem_malloc_id__(listelem_alloc_t *list, char *caller_file,
                       int caller_line, int32 *out_id)
{
    char **block;
    void *ptr;
    size_t ptridx;

    /* Allocate a new block if list empty */
    if (list->freelist == NULL)
	listelem_add_block(list, caller_file, caller_line);

    block = (char **)gnode_ptr(list->blocks);

    /* Unlink and return first element in freelist */
    ptr = list->freelist;
    list->freelist = (char **) (*(list->freelist));
    (list->n_alloc)++;

    ptridx = ((char **)ptr - block) / (list->elemsize / sizeof(void *));
    if (out_id)
        *out_id = ((list->n_blocks - 1) << BLKID_SHIFT) | ptridx;

    return ptr;
}

void *
listelem_get_item(listelem_alloc_t *list, int32 id)
{
    int32 block, ptridx, i;
    gnode_t *gn;

    block = (id >> BLKID_SHIFT) & BLKID_MASK;
    ptridx = id & BLKID_MASK;

    i = list->n_blocks - (block + 1);
    gn = list->blocks;
    while (i > 0) {
        gn = gnode_next(gn);
        --i;
    }

    return (void *)((char **)gnode_ptr(gn)
                    + ptridx * (list->elemsize / sizeof(void *)));
}

void
__listelem_free__(listelem_alloc_t *list, void *elem,
                  char *caller_file, int caller_line)
{
    char **cpp;

    /*
     * Insert freed item at head of list.
     *
     * FIXME: As far as I can tell, this will not allow the freelist
     * to cross blocks.  So it will waste memory (although it makes
     * listitem_malloc_id() simpler this way).
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
    E_INFO("blk_alloc %lu, #blocks %lu\n",
           (unsigned long)list->blk_alloc,
           (unsigned long)list->n_blocks);
    E_INFO("Allocated blocks:\n");
    for (gn = list->blocks; gn; gn = gnode_next(gn))
	E_INFO("%p\n", gnode_ptr(gn));
}
