/*
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

void
cmn_adapt(cmn_t *cmn, mfcc_t **incep, int32 varnorm, int32 nfr) {

    int32 i, j;
    mfcc_t cep_cur[cmn->veclen];        // check it please, is it correctly?

    if (nfr <= 0)
        return;

    for (j = 0; j < cmn->veclen; j++) {
        cep_cur[j] = 0.0f;
        for (i = 0; i < nfr; i++)
            if ( abs(incep[i][j]) > cep_cur[j] )
                cep_cur[j] = abs(incep[i][j]);

        if ( cep_cur[j] == 0.0f )
            cep_cur[j] = 0.0001f;

        if ( cmn->max[j] == 0.0f )
            cmn->max[j] = cep_cur[j] * 1.1f;

        if ( cep_cur[j] > cmn->max[j] * 2.0f ) {

            cmn->max[j] = cep_cur[j] * 1.1f;

        } else {

            mfcc_t u_prob = ( cep_cur[j] / cmn->max[j] ) * 0.1f;

            if ( u_prob > 0.1f )
                u_prob = 0.1f;

            cmn->max[j] = cep_cur[j] * u_prob + cmn->max[j] * (1.0f-u_prob);
        }

    }

    mfcc_t prob0 = ( cep_cur[0] / cmn->max[0] ) * 0.01f;

    if ( prob0 > 0.01 )
        prob0 = 0.01f;

    for (i = 0; i < nfr; i++) {

        if ( cep_cur[0] > cmn->max[0] )
            cmn->max[0] = cep_cur[0] * prob0 + cmn->max[0] * (1.0f-prob0);

        mfcc_t e_prob = incep[i][0] / cmn->max[0] * 0.001f;

        for (j = 0; j < cmn->veclen; j++) {
            cmn->sum[j] += incep[i][j];                         // save compatibility with prior method, you can to remove it
            cmn->cmn_mean[j] = ( incep[i][j] * e_prob ) + ( cmn->cmn_mean[j] * (1.0f-e_prob) );
            incep[i][j] -= cmn->cmn_mean[j];
            if (varnorm)
                incep[i][j] /= cmn->max[j];
        }

        ++cmn->nframe;                                          // save compatibility with prior method, you can to remove it
    }
}
