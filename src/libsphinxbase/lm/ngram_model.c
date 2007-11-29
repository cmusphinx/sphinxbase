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

#include "ngram_model.h"
#include "ngram_model_internal.h"
#include "ckd_alloc.h"
#include "filename.h"
#include "bio.h"
#include "logmath.h"

#include <string.h>

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
    ckd_free(model);
}


int
ngram_model_recode(ngram_model_t *model, const char *from, const char *to)
{
    return -1;
}

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
    return -1;
}

const char *
ngram_word(ngram_model_t *model, int32 wid)
{
    return NULL;
}
