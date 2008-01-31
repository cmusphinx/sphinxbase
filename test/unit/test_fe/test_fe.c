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
	int16 buf[2048], *inptr;
	int32 frame_shift, frame_size;
	mfcc_t cepvec[DEFAULT_NUM_CEPSTRA], *outptr;
	int32 nfr, nsamp;

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

	printf("%d %d %d %d %d\n", frame_size, frame_shift, inptr - buf, nsamp, nfr);
	/* Process the first frame. */
	TEST_EQUAL(0, fe_process_frames(fe, &inptr, &nsamp, &outptr, &nfr));
	/* This is somewhat dependent on the internals - there is at
	 * most frame_shift worth of overflow data, which will be
	 * consumed. */
	TEST_EQUAL(inptr, buf + frame_size + frame_shift);
	TEST_EQUAL(nsamp, 1024 - frame_size - frame_shift);
	TEST_EQUAL(nfr, 1);

	printf("%d %d %d %d %d\n", frame_size, frame_shift, inptr - buf, nsamp, nfr);
	outptr = &cepvec[0];
	TEST_EQUAL(0, fe_process_frames(fe, &inptr, &nsamp, &outptr, &nfr));
	TEST_EQUAL(inptr, buf + frame_size + 2 * frame_shift);
	TEST_EQUAL(nsamp, 1024 - frame_size - 2 * frame_shift);
	TEST_EQUAL(nfr, 1);

	printf("%d %d %d %d %d\n", frame_size, frame_shift, inptr - buf, nsamp, nfr);
	outptr = &cepvec[0];
	TEST_EQUAL(0, fe_process_frames(fe, &inptr, &nsamp, &outptr, &nfr));
	TEST_EQUAL(inptr, buf + frame_size + 3 * frame_shift);
	TEST_EQUAL(nsamp, 1024 - frame_size - 3 * frame_shift);
	TEST_EQUAL(nfr, 1);

	printf("%d %d %d %d %d\n", frame_size, frame_shift, inptr - buf, nsamp, nfr);
	outptr = &cepvec[0];
	TEST_EQUAL(0, fe_process_frames(fe, &inptr, &nsamp, &outptr, &nfr));
	TEST_EQUAL(inptr, buf + 1024);
	TEST_EQUAL(nsamp, 0);
	TEST_EQUAL(nfr, 1);

	printf("%d %d %d %d %d\n", frame_size, frame_shift, inptr - buf, nsamp, nfr);
	TEST_EQUAL(0, fe_end_utt(fe, cepvec, &nfr));
	TEST_EQUAL(1, nfr);

	fclose(raw);
	fe_close(fe);
	cmd_ln_free_r(config);

	return 0;
}
