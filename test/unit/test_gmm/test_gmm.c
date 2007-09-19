#include <gau_cb.h>
#include <gau_mix.h>
#include <strfuncs.h>

#include <stdio.h>

int
main(int argc, char *argv[])
{
	gau_cb_t *cb;
	gau_mix_t *mix;
	char *meanfn, *varfn, *mixwfn;

	if (argc == 1) {
		fprintf(stderr, "Usage: %s HMMDIR\n", argv[0]);
		return 1;
	}

	meanfn = string_join(argv[1], "/means", NULL);
	varfn = string_join(argv[1], "/varfn", NULL);
	mixwfn = string_join(argv[1], "/mixture_weights", NULL);

	cb = gau_cb_read(NULL, meanfn, varfn);
	mix = gau_mix_read(mixwfn);

	gau_cb_free(cb);
	gau_mix_free(mix);

	return 0;
}
