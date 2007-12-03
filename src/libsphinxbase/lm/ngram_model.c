/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2007 Carnegie Mellon University.  All rights
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
 * \file lm_3g.c DARPA Trigram LM module (adapted to Sphinx 3)
 *
 * Author: David Huggins-Daines, much code taken from sphinx3/src/libs3decoder/liblm
 */

#include "config.h"
#include "ngram_model.h"
#include "ngram_model_internal.h"
#include "ckd_alloc.h"
#include "filename.h"
#include "bio.h"
#include "err.h"
#include "logmath.h"

#include <string.h>
#ifdef HAVE_ICONV
#include <iconv.h>
#endif

ngram_file_type_t
ngram_file_name_to_type(const char *file_name)
{
    const char *ext;

    ext = strrchr(file_name, '.');
    if (ext == NULL) {
        return NGRAM_ARPA; /* Default file type */
    }
    if (0 == strcasecmp(ext, ".gz")) {
        while (--ext >= file_name) {
            if (*ext == '.') break;
        }
        if (ext < file_name) {
            return NGRAM_ARPA; /* Default file type */
        }
    }
    if (0 == strncasecmp(ext, ".ARPA", 5))
        return NGRAM_ARPA;
    if (0 == strncasecmp(ext, ".DMP", 4))
        return NGRAM_DMP;
    if (0 == strncasecmp(ext, ".DMP32", 6))
        return NGRAM_DMP32;
    if (0 == strncasecmp(ext, ".FST", 4))
        return NGRAM_FST;
    if (0 == strncasecmp(ext, ".SLF", 4))
        return NGRAM_HTK;
    return NGRAM_ARPA; /* Default file type */
}

ngram_model_t *
ngram_model_read(cmd_ln_t *config,
		 const char *file_name,
                 ngram_file_type_t file_type,
		 logmath_t *lmath)
{
    switch (file_type) {
    case NGRAM_AUTO: {
        ngram_model_t *model;

        if ((model = ngram_model_arpa_read(config, file_name, lmath)) != NULL)
            return model;
        if ((model = ngram_model_dmp_read(config, file_name, lmath)) != NULL)
            return model;
        if ((model = ngram_model_dmp32_read(config, file_name, lmath)) != NULL)
            return model;
        return NULL;
    }
    case NGRAM_ARPA:
        return ngram_model_arpa_read(config, file_name, lmath);
    case NGRAM_DMP:
        return ngram_model_dmp_read(config, file_name, lmath);
    case NGRAM_DMP32:
        return ngram_model_dmp32_read(config, file_name, lmath);
    case NGRAM_FST:
    case NGRAM_HTK:
        return NULL; /* Unsupported format for reading. */
    }

    return NULL; /* In case your compiler is really stupid. */
}

int
ngram_model_write(ngram_model_t *model, const char *file_name,
		  ngram_file_type_t file_type)
{
    switch (file_type) {
    case NGRAM_AUTO: {
        file_type = ngram_file_name_to_type(file_name);
        return ngram_model_write(model, file_name, file_type);
    }
    case NGRAM_ARPA:
        return ngram_model_arpa_write(model, file_name);
    case NGRAM_DMP:
        return ngram_model_dmp_write(model, file_name);
    case NGRAM_DMP32:
        return ngram_model_dmp32_write(model, file_name);
    case NGRAM_FST:
        return ngram_model_fst_write(model, file_name);
    case NGRAM_HTK:
        return ngram_model_htk_write(model, file_name);
    }

    return -1; /* In case your compiler is really stupid. */
}

void
ngram_model_free(ngram_model_t *model)
{
    if (model->funcs && model->funcs->free)
        (*model->funcs->free)(model);
    if (model->writable) {
        int i;
        for (i = 0; i < model->n_counts[0]; ++i) {
            ckd_free(model->word_str[i]);
        }
    }
    ckd_free(model->word_str);
    ckd_free(model);
}


#ifdef HAVE_ICONV
int
ngram_model_recode(ngram_model_t *model, const char *from, const char *to)
{
    iconv_t ic;
    char *outbuf;
    size_t maxlen;
    int i, writable;
    hash_table_t *new_wid;

    /* FIXME: Need to do a special case thing for the GB-HEX encoding
     * used in Sphinx3 Mandarin models. */
    if ((ic = iconv_open(to, from)) == (iconv_t)-1) {
        E_ERROR_SYSTEM("iconv_open() failed");
        return -1;
    }
    /* iconv(3) is a piece of crap and won't accept a NULL out buffer,
     * unlike wcstombs(3). So we have to either call it over and over
     * again until our buffer is big enough, or call it with a huge
     * buffer and then copy things back to the output.  We will use a
     * mix of these two approaches here.  We'll keep a single big
     * buffer around, and expand it as necessary.
     */
    maxlen = 0;
    for (i = 0; i < model->n_counts[0]; ++i) {
        if (strlen(model->word_str[i]) > maxlen)
            maxlen = strlen(model->word_str[i]);
    }
    /* Were word strings already allocated? */
    writable = model->writable;
    /* Either way, we are going to allocate some word strings. */
    model->writable = TRUE;
    /* Really should be big enough except for pathological cases. */
    maxlen = maxlen * sizeof(int) + 15;
    outbuf = ckd_calloc(maxlen, 1);
    /* And, don't forget, we need to rebuild the word to unigram ID
     * mapping. */
    new_wid = hash_table_new(model->n_counts[0], FALSE);
    for (i = 0; i < model->n_counts[0]; ++i) {
        ICONV_CONST char *in;
        char *out;
        size_t inleft, outleft, result;

    start_conversion:
        in = (ICONV_CONST char *)model->word_str[i];
        /* Yes, this assumes that we don't have any NUL bytes. */
        inleft = strlen(in);
        out = outbuf;
        outleft = maxlen;

        while ((result = iconv(ic, &in, &inleft, &out, &outleft)) == (size_t)-1) {
            if (errno != E2BIG) {
                /* FIXME: if we already converted any words, then they
                 * are going to be in an inconsistent state. */
                E_ERROR_SYSTEM("iconv() failed");
                ckd_free(outbuf);
                hash_table_free(new_wid);
                return -1;
            }
            /* Reset the internal state of conversion. */
            iconv(ic, NULL, NULL, NULL, NULL);
            /* Make everything bigger. */
            maxlen *= 2;
            out = outbuf = ckd_realloc(outbuf, maxlen);
            /* Reset the input pointers. */
            in = (ICONV_CONST char *)model->word_str[i];
            inleft = strlen(in);
        }

        /* Now flush a shift-out sequence, if any. */
        if ((result = iconv(ic, NULL, NULL, &out, &outleft)) == (size_t)-1) {
            if (errno != E2BIG) {
                /* FIXME: if we already converted any words, then they
                 * are going to be in an inconsistent state. */
                E_ERROR_SYSTEM("iconv() failed (state reset sequence)");
                ckd_free(outbuf);
                hash_table_free(new_wid);
                return -1;
            }
            /* Reset the internal state of conversion. */
            iconv(ic, NULL, NULL, NULL, NULL);
            /* Make everything bigger. */
            maxlen *= 2;
            outbuf = ckd_realloc(outbuf, maxlen);
            /* Be very evil. */
            goto start_conversion;
        }

        result = maxlen - outleft;
        /* Okay, that was hard, now let's go shopping. */
        if (writable) {
            /* Grow or shrink the output string as necessary. */
            model->word_str[i] = ckd_realloc(model->word_str[i], result + 1);
            model->word_str[result] = '\0';
        }
        else {
            /* It actually was not allocated previously, so do that now. */
            model->word_str[i] = ckd_calloc(result + 1, 1);
        }
        /* Copy the new thing in. */
        memcpy(model->word_str[i], outbuf, result);

        /* Now update the hash table.  We might have terrible
         * collisions if a non-reversible conversion was requested.,
         * so warn about them. */
        if (hash_table_enter(new_wid, model->word_str[i],
                             (void *)i) != (void *)i) {
            E_WARN("Duplicate word in dictionary after conversion: %s\n",
                   model->word_str[i]);
        }
    }
    ckd_free(outbuf);
    iconv_close(ic);
    /* Swap out the hash table. */
    hash_table_free(model->wid);
    model->wid = new_wid;

    return 0;
}
#else /* !HAVE_ICONV */
int
ngram_model_recode(ngram_model_t *model, const char *from, const char *to)
{
    return -1;
}
#endif /* !HAVE_ICONV */

int
ngram_apply_weights(ngram_model_t *model,
		    float32 lw, float32 wip, float32 uw)
{
    return -1;
}

int32
ngram_score(ngram_model_t *model, const char *word, ...)
{
    return NGRAM_SCORE_ERROR;
}

int32
ngram_score_v(ngram_model_t *model, const char *word, va_list history)
{
    return NGRAM_SCORE_ERROR;
}

int32
ngram_tg_score(ngram_model_t *model, int32 w3, int32 w2, int32 w1)
{
    return NGRAM_SCORE_ERROR;
}

int32
ngram_bg_score(ngram_model_t *model, int32 w2, int32 w1)
{
    return NGRAM_SCORE_ERROR;
}

int32
ngram_prob(ngram_model_t *model, const char *word, ...)
{
    return NGRAM_SCORE_ERROR;
}

int32
ngram_prob_v(ngram_model_t *model, const char *word, va_list history)
{
    return NGRAM_SCORE_ERROR;
}

int32
ngram_wid(ngram_model_t *model, const char *word)
{
    void *val;

    if (hash_table_lookup(model->wid, word, &val) == -1)
        return -1;
    else
        return (int32)val;
}

const char *
ngram_word(ngram_model_t *model, int32 wid)
{
    if (wid >= model->n_counts[0])
        return NULL;
    return model->word_str[wid];
}
