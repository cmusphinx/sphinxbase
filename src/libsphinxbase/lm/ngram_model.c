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
#include "pio.h"
#include "err.h"
#include "logmath.h"
#include "strfuncs.h"

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

int32
ngram_model_init(ngram_model_t *base,
                 ngram_funcs_t *funcs,
                 logmath_t *lmath,
                 int32 n, int32 n_unigram)
{
    base->funcs = funcs;
    base->lmath = lmath;
    base->n = n;
    base->n_counts = ckd_calloc(3, sizeof(*base->n_counts));
    base->n_1g_alloc = base->n_words = n_unigram;
    /* Set default values for weights. */
    base->lw = 1.0;
    base->log_wip = 0; /* i.e. 1.0 */
    base->log_uw = 0;  /* i.e. 1.0 */
    base->log_uniform = logmath_log(lmath, 1.0 / n_unigram);
    base->log_uniform_weight = logmath_get_zero(lmath);
    /* Allocate space for word strings. */
    base->word_str = ckd_calloc(n_unigram, sizeof(char *));
    /* NOTE: They are no longer case-insensitive since we are allowing
     * other encodings for word strings.  Beware. */
    base->wid = hash_table_new(n_unigram, FALSE);

    return 0;
}

void
ngram_model_free(ngram_model_t *model)
{
    if (model->funcs && model->funcs->free)
        (*model->funcs->free)(model);
    if (model->writable) {
        int i;
        for (i = 0; i < model->n_words; ++i) {
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
    for (i = 0; i < model->n_words; ++i) {
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
    new_wid = hash_table_new(model->n_words, FALSE);
    for (i = 0; i < model->n_words; ++i) {
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
                             (void *)(long)i) != (void *)(long)i) {
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
    return (*model->funcs->apply_weights)(model, lw, wip, uw);
}

int32
ngram_ng_score(ngram_model_t *model, int32 wid, int32 *history,
               int32 n_hist, int32 *n_used)
{
    int32 score, class_weight = 0;
    int i;

    /* "Declassify" wid and history */
    if (wid != NGRAM_INVALID_WID && NGRAM_IS_CLASSWID(wid)) {
        ngram_class_t *lmclass = model->classes[NGRAM_CLASSID(wid)];

        class_weight = ngram_class_prob(lmclass, wid);
        if (class_weight == NGRAM_SCORE_ERROR)
            return class_weight;
        wid = lmclass->tag_wid;
    }
    for (i = 0; i < n_hist; ++i) {
        if (history[i] != NGRAM_INVALID_WID && NGRAM_IS_CLASSWID(history[i]))
            history[i] = model->classes[NGRAM_CLASSID(history[i])]->tag_wid;
    }
    score = (*model->funcs->score)(model, wid, history, n_hist, n_used);

    /* Multiply by unigram in-class weight. */
    return score + class_weight;
}

int32
ngram_score(ngram_model_t *model, const char *word, ...)
{
    va_list history;
    const char *hword;
    int32 *histid;
    int32 n_hist;
    int32 n_used;
    int32 prob;

    va_start(history, word);
    n_hist = 0;
    while ((hword = va_arg(history, const char *)) != NULL)
        ++n_hist;
    va_end(history);

    histid = ckd_calloc(n_hist, sizeof(*histid));
    va_start(history, word);
    n_hist = 0;
    while ((hword = va_arg(history, const char *)) != NULL) {
        histid[n_hist] = ngram_wid(model, hword);
        ++n_hist;
    }
    va_end(history);

    prob = ngram_ng_score(model, ngram_wid(model, word),
                          histid, n_hist, &n_used);
    ckd_free(histid);
    return prob;
}

int32
ngram_tg_score(ngram_model_t *model, int32 w3, int32 w2, int32 w1, int32 *n_used)
{
    int32 hist[2] = { w2, w1 };
    return ngram_ng_score(model, w3, hist, 2, n_used);
}

int32
ngram_bg_score(ngram_model_t *model, int32 w2, int32 w1, int32 *n_used)
{
    return ngram_ng_score(model, w2, &w1, 1, n_used);
}

int32
ngram_ng_prob(ngram_model_t *model, int32 wid, int32 *history,
              int32 n_hist, int32 *n_used)
{
    int32 prob, class_weight = 0;
    int i;

    /* "Declassify" wid and history */
    if (wid != NGRAM_INVALID_WID && NGRAM_IS_CLASSWID(wid)) {
        ngram_class_t *lmclass = model->classes[NGRAM_CLASSID(wid)];

        class_weight = ngram_class_prob(lmclass, wid);
        if (class_weight == NGRAM_SCORE_ERROR)
            return class_weight;
        wid = lmclass->tag_wid;
    }
    for (i = 0; i < n_hist; ++i) {
        if (history[i] != NGRAM_INVALID_WID && NGRAM_IS_CLASSWID(history[i]))
            history[i] = model->classes[NGRAM_CLASSID(history[i])]->tag_wid;
    }
    prob = (*model->funcs->raw_score)(model, wid, history,
                                      n_hist, n_used);
    /* Multiply by unigram in-class weight. */
    return prob + class_weight;
}

int32
ngram_prob(ngram_model_t *model, const char *word, ...)
{
    va_list history;
    const char *hword;
    int32 *histid;
    int32 n_hist;
    int32 n_used;
    int32 prob;

    va_start(history, word);
    n_hist = 0;
    while ((hword = va_arg(history, const char *)) != NULL)
        ++n_hist;
    va_end(history);

    histid = ckd_calloc(n_hist, sizeof(*histid));
    va_start(history, word);
    n_hist = 0;
    while ((hword = va_arg(history, const char *)) != NULL) {
        histid[n_hist] = ngram_wid(model, hword);
        ++n_hist;
    }
    va_end(history);

    prob = ngram_ng_prob(model, ngram_wid(model, word),
                         histid, n_hist, &n_used);
    ckd_free(histid);
    return prob;
}

int32
ngram_wid(ngram_model_t *model, const char *word)
{
    void *val;

    if (hash_table_lookup(model->wid, word, &val) == -1)
        return -1;
    else
        return (int32)(long)val;
}

const char *
ngram_word(ngram_model_t *model, int32 wid)
{
    /* Remove any class tag */
    wid = NGRAM_BASEWID(wid);
    if (wid >= model->n_words)
        return NULL;
    return model->word_str[wid];
}

/**
 * Add a word to the word string and ID mapping.
 */
int32
ngram_add_word_internal(ngram_model_t *model,
                        const char *word,
                        int32 classid)
{
    void *dummy;
    int32 wid;

    /* Take the next available word ID */
    wid = model->n_words;
    if (classid >= 0) {
        wid = NGRAM_CLASSWID(wid, classid);
    }
    /* Check for hash collisions. */
    if (hash_table_lookup(model->wid, word, &dummy) == 0) {
        E_ERROR("Duplicate definition of word %s\n", word);
        return NGRAM_INVALID_WID;
    }
    /* Reallocate word_str if necessary. */
    if (model->n_words >= model->n_1g_alloc) {
        model->n_1g_alloc += UG_ALLOC_STEP;
        model->word_str = ckd_realloc(model->word_str,
                                      sizeof(*model->word_str) * model->n_1g_alloc);
    }
    /* Add the word string in the appropriate manner. */
    /* Class words are always dynamically allocated (FIXME: memory leak...) */
    model->word_str[model->n_words] = ckd_salloc(word);
    /* Now enter it into the hash table. */
    if (hash_table_enter(model->wid, model->word_str[model->n_words],
                         (void *)(long)(wid)) != (void *)(long)(wid)) {
        E_ERROR("Hash insertion failed for word %s => %p (should not happen)\n",
                model->word_str[model->n_words], (void *)(long)(wid));
    }
    /* Increment number of words. */
    ++model->n_words;
    return wid;
}

int32
ngram_add_word(ngram_model_t *model,
               const char *word, float32 weight)
{
    int32 wid, prob = NGRAM_SCORE_ERROR;

    wid = ngram_add_word_internal(model, word, -1);
    if (wid == NGRAM_INVALID_WID)
        return wid;

    /* Do what needs to be done to add the word to the unigram. */
    if (model->funcs && model->funcs->add_ug)
        prob = (*model->funcs->add_ug)(model, wid, logmath_log(model->lmath, weight));
    if (prob == NGRAM_SCORE_ERROR) {
        if (model->writable)
            ckd_free(model->word_str[wid]);
        return -1;
    }
    return wid;
}

ngram_class_t *
ngram_class_new(ngram_model_t *model, int32 tag_wid, int32 start_wid, glist_t classwords)
{
    ngram_class_t *lmclass;
    gnode_t *gn;
    float32 tprob;
    int i;

    lmclass = ckd_calloc(1, sizeof(*lmclass));
    lmclass->tag_wid = tag_wid;
    classwords = glist_reverse(classwords);
    /* wid_base is the wid (minus class tag) of the first word in the list. */
    lmclass->start_wid = start_wid;
    lmclass->n_words = glist_count(classwords);
    lmclass->prob1 = ckd_calloc(lmclass->n_words, sizeof(*lmclass->prob1));
    lmclass->nword_hash = NULL;
    lmclass->n_hash = 0;
    tprob = 0.0;
    for (gn = classwords; gn; gn = gnode_next(gn)) {
        tprob += gnode_float32(gn);
    }
    if (tprob > 1.1 || tprob < 0.9) {
        E_WARN("Total class probability is %f, will normalize\n", tprob);
        for (gn = classwords; gn; gn = gnode_next(gn)) {
            gnode_float32(gn) = gnode_float32(gn) / tprob;
        }
    }
    for (i = 0, gn = classwords; gn; ++i, gn = gnode_next(gn)) {
        lmclass->prob1[i] = logmath_log(model->lmath, gnode_float32(gn));
    }

    return lmclass;
}

void
ngram_class_free(ngram_class_t *lmclass)
{
    ckd_free(lmclass->nword_hash);
    ckd_free(lmclass->prob1);
    ckd_free(lmclass);
}

int32
ngram_class_prob(ngram_class_t *lmclass, int32 wid)
{
    int32 base_wid = NGRAM_BASEWID(wid);

    if (base_wid < lmclass->start_wid
        || base_wid > lmclass->start_wid + lmclass->n_words) {
        /* FIXME: Look it up in the hash table.  Just fail for now. */
        return NGRAM_SCORE_ERROR;
    }
    return lmclass->prob1[base_wid - lmclass->start_wid];
}

int32
ngram_model_read_classdef(ngram_model_t *model,
                          const char *file_name)
{
    FILE *fp;
    int32 is_pipe;
    int inclass;  /**< Are we currently reading a list of class words? */
    int classid;  /**< ID of class currently under construction. */
    int start_wid;/**< Start word ID of current class. */
    int n_classes;/**< Number of classes created. */
    char *classname = NULL; /**< Name of current class. */
    int32 classwid = -1;    /**< Base word ID of current class tag. */
    glist_t classes = NULL; /**< List of classes read from this file. */
    glist_t classwords = NULL; /**< List of words read for current class. */
    gnode_t *gn;

    if ((fp = fopen_comp(file_name, "r", &is_pipe)) == NULL) {
        E_ERROR("File %s not found\n", file_name);
        return -1;
    }

    classid = model->n_classes;
    inclass = 0;
    start_wid = model->n_words;
    while (!feof(fp)) {
        char line[512];
        char *wptr[2];
        int n_words;

        if (fgets(line, sizeof(line), fp) == NULL)
            break;

        n_words = str2words(line, wptr, 2);
        if (n_words <= 0)
            continue;

        if (inclass) {
            /* Look for an end of class marker. */
            if (n_words == 2 && 0 == strcmp(wptr[0], "END")) {
                ngram_class_t *lmclass;

                if (classname == NULL || 0 != strcmp(wptr[1], classname))
                    goto error_out;
                inclass = FALSE;
                /* Construct a class from the list of words collected. */
                lmclass = ngram_class_new(model, classwid, start_wid, classwords);
                ckd_free(classname);
                classname = NULL;
                classwid = -1;
                /* Reset the list of words collected. */
                glist_free(classwords);
                classwords = NULL;
                /* Add this class to the list of classes collected. */
                classes = glist_add_ptr(classes, lmclass);
                
                ++classid;
            }
            else {
                float32 fprob;

                if (n_words == 2)
                    fprob = atof(wptr[1]);
                else
                    fprob = 1.0f;
                /* Add this word to the language model. */
                if (ngram_add_word_internal(model, wptr[0], classid) == NGRAM_INVALID_WID)
                    goto error_out;
                /* Add it to the list of words for this class. */
                classwords = glist_add_float32(classwords, fprob);
            }
        }
        else {
            /* Start a new LM class if the LMCLASS marker is seen */
            if (n_words == 2 && 0 == strcmp(wptr[0], "LMCLASS")) {
                if (inclass)
                    goto error_out;
                inclass = TRUE;
                classname = ckd_salloc(wptr[1]);
                if ((classwid = ngram_wid(model, classname)) == NGRAM_INVALID_WID) {
                    E_ERROR("Class tag %s not present in unigram\n", classname);
                    goto error_out;
                }
                start_wid = model->n_words;
            }
            /* Otherwise, just ignore whatever junk we got */
        }
    }

    /* Take the list of classes and add it to whatever might have
     * already been in the language model. */
    classes = glist_reverse(classes);
    n_classes = glist_count(classes);
    if (model->n_classes + n_classes > 128) {
        E_ERROR("Number of classes cannot exceed 128 (sorry)\n");
        goto error_out;
    }
    if (model->classes == NULL) {
        model->classes = ckd_calloc(n_classes,
                                    sizeof(*model->classes));
    }
    else {
        model->classes = ckd_realloc(model->classes + n_classes,
                                     model->n_classes * sizeof(*model->classes));
    }
    for (gn = classes; gn; gn = gnode_next(gn)) {
        model->classes[model->n_classes] = gnode_ptr(gn);
        ++model->n_classes;
    }
    glist_free(classes);
    fclose_comp(fp, is_pipe);
    return 0;

error_out:
    /* Free all the stuff we might have allocated. */
    glist_free(classwords);
    for (gn = classes; gn; gn = gnode_next(gn)) {
        ngram_class_free(gnode_ptr(gn));
    }
    glist_free(classes);
    ckd_free(classname);
    return -1;
}
