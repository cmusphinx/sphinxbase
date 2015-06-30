#ifndef __NGRAM_MODEL_TRIE_H__
#define __NGRAM_MODEL_TRIE_H__

#include <sphinxbase/prim_type.h>
#include <sphinxbase/logmath.h>

#include "ngram_model_internal.h"
#include "lm_trie.h"

typedef struct ngram_model_trie_s {
    ngram_model_t base;  /**< Base ngram_model_t structure */
    lm_trie_t *trie;     /**< Trie structure that stores ngram relations and weights */
}ngram_model_trie_t;

#endif /* __NGRAM_MODEL_TRIE_H__ */