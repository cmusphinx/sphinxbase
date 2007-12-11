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
 * \file lm3g_model.c Core Sphinx 3-gram code used in
 * DMP/DMP32/ARPA (for now) model code.
 *
 * Author: A cast of thousands, probably.
 */
#include "lm3g_model.h"
#include "linklist.h"
#include "ckd_alloc.h"

#include <string.h>

void
lm3g_tginfo_free(ngram_model_t *base, tginfo_t **tginfo)
{
        int32 u;
	if (tginfo == NULL)
		return;
        for (u = 0; u < base->n_1g_alloc; u++) {
		tginfo_t *t, *next_t;
		for (t = tginfo[u]; t; t = next_t) {
			next_t = t->next;
			listelem_free(t, sizeof(*t));
		}
        }
        ckd_free(tginfo);
}

void
lm3g_apply_weights(ngram_model_t *base,
		   lm3g_model_t *lm3g,
		   float32 lw, float32 wip, float32 uw)
{
    int32 log_wip, log_uw, log_uniform;
    int i;

    /* Precalculate some log values we will like. */
    log_wip = logmath_log(base->lmath, wip);
    log_uw = logmath_log(base->lmath, uw);
    /* Log of (1-uw) / N_unigrams */
    log_uniform = logmath_log(base->lmath, 1.0 / (base->n_counts[0] - 1))
        + logmath_log(base->lmath, 1.0 - uw);

    for (i = 0; i < base->n_counts[0]; ++i) {
        lm3g->unigrams[i].bo_wt1.l = (int32)(lm3g->unigrams[i].bo_wt1.l * lw);

        if (strcmp(base->word_str[i], "<s>") == 0) { /* FIXME: configurable start_sym */
            /* Apply language weight and WIP */
            lm3g->unigrams[i].prob1.l = (int32)(lm3g->unigrams[i].prob1.l * lw) + log_wip;
        }
        else {
            /* Interpolate unigram probability with uniform. */
            lm3g->unigrams[i].prob1.l += log_uw;
            lm3g->unigrams[i].prob1.l =
                logmath_add(base->lmath,
                            lm3g->unigrams[i].prob1.l,
                            log_uniform);
            /* Apply language weight and WIP */
            lm3g->unigrams[i].prob1.l = (int32)(lm3g->unigrams[i].prob1.l * lw) + log_wip;
        }
    }

    for (i = 0; i < lm3g->n_prob2; ++i) {
        lm3g->prob2[i].l = (int32)(lm3g->prob2[i].l * lw) + log_wip;
    }

    if (base->n > 2) {
        for (i = 0; i < lm3g->n_bo_wt2; ++i) {
            lm3g->bo_wt2[i].l = (int32)(lm3g->bo_wt2[i].l * lw);
        }
        for (i = 0; i < lm3g->n_prob3; i++) {
            lm3g->prob3[i].l = (int32)(lm3g->prob3[i].l * lw) + log_wip;
        }
    }

    /* Store said values in the model so that we will be able to
     * recover the original probs. */
    base->log_wip = log_wip;
    base->log_uniform = log_uniform;
    base->log_uw = log_uw;
    base->lw = lw;
}
