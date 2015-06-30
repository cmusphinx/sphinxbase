#ifndef __LM_TRIE_H__
#define __LM_TRIE_H__

#include <sphinxbase/pio.h>
#include <sphinxbase/bitarr.h>

#include "ngram_model_internal.h"
#include "lm_trie_quant.h"

typedef struct unigram_s {
    float prob;
    float bo;
    uint32 next;
}unigram_t;

typedef struct node_range_s {
    uint32 begin;
    uint32 end;
}node_range_t;

typedef struct base_s {
    uint8 word_bits;
    uint8 total_bits;
    uint32 word_mask;
    uint8 *base;
    uint32 insert_index;
    uint32 max_vocab;
}base_t;

typedef struct middle_s {
    base_t base;
    bitarr_mask_t next_mask; 
    uint8 quant_bits;
    void *next_source;
}middle_t;

typedef struct longest_s {
    base_t base;
    uint8 quant_bits;
}longest_t;

typedef struct lm_trie_s {
    uint8 *ngram_mem;
    size_t ngram_mem_size;
    unigram_t *unigrams;
    middle_t *middle_begin;
    middle_t *middle_end;
    longest_t *longest;
    lm_trie_quant_t *quant;

    float backoff[NGRAM_MAX_ORDER];
    uint32 prev_hist[NGRAM_MAX_ORDER - 1];
}lm_trie_t;

/**
 * Creates lm_trie structure. Fills it if binary file with correspondent data is provided
 */
lm_trie_t* lm_trie_create(uint32 unigram_count, lm_trie_quant_type_t quant_type, int order);

lm_trie_t* lm_trie_read_bin(uint32* counts, int order, FILE *fp);

void lm_trie_write_bin(lm_trie_t *trie, uint32 unigram_count, FILE *fp);

void lm_trie_free(lm_trie_t *trie);

void lm_trie_alloc_ngram(lm_trie_t *trie, uint32 *counts, int order);

void lm_trie_build(lm_trie_t *trie, ngram_raw_t **raw_ngrams, uint32 *counts, int order);

unigram_t* unigram_find(unigram_t *u, uint32 word, node_range_t *next);

float lm_trie_score(lm_trie_t *trie, int order, int32 wid, int32 *hist, int32 n_hist, int32 *n_used);

#endif /* __LM_TRIE_H__ */
