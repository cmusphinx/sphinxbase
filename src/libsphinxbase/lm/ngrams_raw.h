#ifndef __NGRAMS_RAW_H__
#define __NGRAMS_RAW_H__

#include <sphinxbase/hash_table.h>
#include <sphinxbase/logmath.h>
#include <sphinxbase/prim_type.h>
#include <sphinxbase/pio.h>
#include <sphinxbase/err.h>

typedef struct ngram_raw_s {
    uint32 *words; /* array of word indexes, length corresponds to ngram order */
    float *weights;  /* prob and backoff or just prob for longest order */
}ngram_raw_t;

typedef struct ngram_raw_ord_s {
    ngram_raw_t instance;
    int order;
} ngram_raw_ord_t;

typedef union {
    float32 f;
    int32 l;
} dmp_weight_t;

/**
 * Raw ngrams comparator. Usage:
 * > ngram_comparator(NULL, &order); - to set order of ngrams
 * > qsort(ngrams, count, sizeof(lm_ngram_t), &ngram_comparator); - to sort ngrams in increasing order
 */
int ngram_comparator(const void *first_void, const void *second_void);

/**
 * Raw ordered ngrams comparator
 */
int ngram_ord_comparator(void *a_raw, void *b_raw);

/**
 * Read ngrams of order > 1 from ARPA file
 * @param li     [in] sphinxbase file line iterator that point to bigram description in ARPA file
 * @param wid    [in] hashtable that maps string word representation to id
 * @param lmath  [in] log math used for log convertions
 * @param counts [in] amount of ngrams for each order
 * @param order  [in] maximum order of ngrams
 * @return            raw ngrams of order bigger than 1
 */
ngram_raw_t** ngrams_raw_read_arpa(lineiter_t **li, logmath_t *lmath, uint32 *counts, int order, hash_table_t *wid);

/**
 * Reads ngrams of order > 1 from DMP file.
 * @param fp           [in] file to read from. Position in file corresponds to start of bigram description
 * @param lmath        [in] log math used for log convertions
 * @param counts       [in] amount of ngrams for each order
 * @param order        [in] maximum order of ngrams
 * @param unigram_next [in] array of next word pointers for unigrams. Needed to define forst word of bigrams
 * @param do_swap      [in] wether to do swap of bits
 * @return                  raw ngrams of order bigger than 1
 */
ngram_raw_t** ngrams_raw_read_dmp(FILE *fp, logmath_t *lmath, uint32 *counts, int order, uint32 *unigram_next, uint8 do_swap);

void ngrams_raw_fix_counts(ngram_raw_t **raw_ngrams, uint32 *counts, uint32 *fixed_counts, int order);

void ngrams_raw_free(ngram_raw_t **raw_ngrams, uint32 *counts, int order);

#endif /* __LM_NGRAMS_RAW_H__ */