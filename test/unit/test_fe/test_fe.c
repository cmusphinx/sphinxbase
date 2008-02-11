#include <stdio.h>

#include "fe.h"
#include "cmd_ln.h"

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	static const arg_t fe_args[] = {
		waveform_to_cepstral_command_line_macro(),
		{ NULL, 0, NULL, NULL }
	};
	FILE *raw;
	cmd_ln_t *config;
	fe_t *fe;
	int16 buf[2048];
	int16 const *inptr;
	int32 frame_shift, frame_size;
	mfcc_t cepvec[DEFAULT_NUM_CEPSTRA], *outptr;
	int32 nfr;
	size_t nsamp;

	TEST_ASSERT(config = cmd_ln_parse_r(NULL, fe_args, argc, argv, FALSE));
	TEST_ASSERT(fe = fe_init_auto_r(config));

	TEST_EQUAL(fe_get_output_size(fe), DEFAULT_NUM_CEPSTRA);

	fe_get_input_size(fe, &frame_shift, &frame_size);
	TEST_EQUAL(frame_shift, DEFAULT_FRAME_SHIFT);
	TEST_EQUAL(frame_size, (int)(DEFAULT_WINDOW_LENGTH*DEFAULT_SAMPLING_RATE));

	TEST_ASSERT(raw = fopen(DATADIR "/chan3.raw", "rb"));

	TEST_EQUAL(0, fe_start_utt(fe));
	TEST_EQUAL(1024, fread(buf, sizeof(int16), 1024, raw));

	nsamp = 1024;
	TEST_EQUAL(0, fe_process_frames(fe, NULL, &nsamp, NULL, &nfr));
	TEST_EQUAL(1024, nsamp);
	TEST_EQUAL(4, nfr);

	inptr = &buf[0];
	outptr = &cepvec[0];
	nfr = 1;

	printf("frame_size %d frame_shift %d\n", frame_size, frame_shift);
	/* Process the first frame. */
	TEST_EQUAL(0, fe_process_frames(fe, &inptr, &nsamp, &outptr, &nfr));
	printf("inptr %d nsamp %d nfr %d\n", inptr - buf, nsamp, nfr);
	TEST_EQUAL(nfr, 1);

	/* Note that this next one won't actually consume any frames
	 * of input, because it already got sufficient overflow
	 * samples last time around.  This is implementation-dependent
	 * so we shouldn't actually test for it. */
	outptr = &cepvec[0];
	TEST_EQUAL(0, fe_process_frames(fe, &inptr, &nsamp, &outptr, &nfr));
	printf("inptr %d nsamp %d nfr %d\n", inptr - buf, nsamp, nfr);
	TEST_EQUAL(nfr, 1);

	outptr = &cepvec[0];
	TEST_EQUAL(0, fe_process_frames(fe, &inptr, &nsamp, &outptr, &nfr));
	printf("inptr %d nsamp %d nfr %d\n", inptr - buf, nsamp, nfr);
	TEST_EQUAL(nfr, 1);

	outptr = &cepvec[0];
	TEST_EQUAL(0, fe_process_frames(fe, &inptr, &nsamp, &outptr, &nfr));
	printf("inptr %d nsamp %d nfr %d\n", inptr - buf, nsamp, nfr);
	TEST_EQUAL(nfr, 1);

	TEST_EQUAL(0, fe_end_utt(fe, cepvec, &nfr));
	printf("nfr %d\n", nfr);
	TEST_EQUAL(nfr, 1);

	fclose(raw);
	fe_close(fe);
	cmd_ln_free_r(config);

	return 0;
}
