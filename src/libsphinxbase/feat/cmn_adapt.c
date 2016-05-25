/*
 * Warning: supporting only floating point operations
 *
 * 16-Feb-2016  Zamir Ostroukhov <zamiron@gmail.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef _MSC_VER
#pragma warning (disable: 4244)
#endif

#include "sphinxbase/ckd_alloc.h"
#include "sphinxbase/cmn.h"

/* You can change it for best result */
#define ZERO_SUBSTITUTION       0.0000001f
#define COEF_MEAN_PER_FRAME     0.001f  /* speed ratio adaptation for mean      (per frame) */
#define COEF_POWER_PER_FRAME    0.01f   /* speed ratio adaptation for max power (per frame) */
#define COEF_MAX_PER_CALL       0.1f    /* speed ratio adaptation for maximums  (per function call) */
#define COEF_MAX_PER_ERROR      1.1f    /* speed ratio adaptation for maximums, when an error is detected (per function call) */
#define THRESHOLD_MAX_ERROR     2.0f    /* threshold for error detector */

void
cmn_adapt(cmn_t *cmn, mfcc_t **incep, int32 varnorm, int32 nfr) {

    int32 i, j;

    if (nfr <= 0)
        return;

    for (j = 0; j < cmn->veclen; j++) {
        cmn->cur[j] = 0.0f;
        for (i = 0; i < nfr; i++)
            if ( abs(incep[i][j]) > cmn->cur[j] )
                cmn->cur[j] = abs(incep[i][j]);

        if ( cmn->cur[j] == 0.0f )
            cmn->cur[j] = ZERO_SUBSTITUTION;

        if ( cmn->max[j] == 0.0f )
            cmn->max[j] = cmn->cur[j] * COEF_MAX_PER_ERROR;

        if ( cmn->cur[j] > cmn->max[j] * THRESHOLD_MAX_ERROR ) {

            cmn->max[j] = cmn->cur[j] * COEF_MAX_PER_ERROR;

        } else {

            mfcc_t u_prob = ( cmn->cur[j] / cmn->max[j] ) * COEF_MAX_PER_CALL;

            if ( u_prob > COEF_MAX_PER_CALL )
                u_prob = COEF_MAX_PER_CALL;

            cmn->max[j] = cmn->cur[j] * u_prob + cmn->max[j] * (1.0f-u_prob);
        }

    }

    mfcc_t prob0 = ( cmn->cur[0] / cmn->max[0] ) * COEF_POWER_PER_FRAME;

    if ( prob0 > COEF_POWER_PER_FRAME )
        prob0 = COEF_POWER_PER_FRAME;

    for (i = 0; i < nfr; i++) {

        if ( cmn->cur[0] > cmn->max[0] )
            cmn->max[0] = cmn->cur[0] * prob0 + cmn->max[0] * (1.0f-prob0);

        mfcc_t e_prob;

        if ( incep[i][0] <= 0.0f ) {
            e_prob = 0.0f;
        } else {
            e_prob = incep[i][0] / cmn->max[0] * COEF_MEAN_PER_FRAME;
        }

        for (j = 0; j < cmn->veclen; j++) {
            cmn->sum[j] += incep[i][j];                         // save compatibility with prior method, you can to remove it
            cmn->cmn_mean[j] = incep[i][j] * e_prob + cmn->cmn_mean[j] * (1.0f-e_prob);
            incep[i][j] -= cmn->cmn_mean[j];
            if (varnorm)
                incep[i][j] /= cmn->max[j];
        }

        ++cmn->nframe;                                          // save compatibility with prior method, you can to remove it
    }
}
