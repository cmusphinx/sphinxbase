#include "config.h"
#include "ad.h"
#include "cont_ad.h"
#include "byteorder.h"
#include "test_macros.h"

#include <stdio.h>

#ifndef WORDS_BIGENDIAN
#define WORDS_BIGENDIAN 0
#endif

int
main(int argc, char *argv[])
{
    cont_ad_t *cont;
    FILE *infp;
    int16 buf[512];
    int listening;
    int k;

    TEST_ASSERT(infp = fopen(DATADIR "/chan3.raw", "rb"));
    TEST_ASSERT(cont = cont_ad_init(NULL, NULL));

    printf("Calibrating ...");
    fflush(stdout);
    while ((k = fread(buf, 2, 512, infp)) > 0) {
	int rv = cont_ad_calib_loop(cont, buf, k);
	if (rv < 0)
	    printf(" failed; file too short?\n");
	else if (rv == 0) {
	    printf(" done after %ld samples\n", ftell(infp));
	    break;
	}
    }
    rewind(infp);

    listening = FALSE;
    while (1) {
	k = fread(buf, 2, 512, infp);

	/* End of file. */
	if (k < 256) {
	    if (listening) {
		printf("End of file at %.3f seconds\n",
		       (double)(cont->read_ts - k) / 16000);
	    }
	    break;
	}

	k = cont_ad_read(cont, buf, k);

	if (cont->state == CONT_AD_STATE_SIL) {
	    /* Has there been enough silence to cut the utterance? */
	    if (listening && cont->seglen > 8000) {
		printf("End of utterance at %.3f seconds\n",
		       (double)(cont->read_ts - k - cont->seglen) / 16000);
		listening = FALSE;
	    }
	}
	else {
	    if (!listening) {
		printf("Start of utterance at %.3f seconds\n",
		       (double)(cont->read_ts - k) / 16000);
		listening = TRUE;
	    }
	}
    }

    fclose(infp);
    return 0;
}
