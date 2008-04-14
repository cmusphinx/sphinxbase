#include <jsgf.h>
#include <fsg_model.h>

#include "test_macros.h"

int
main(int argc, char *argv[])
{
	logmath_t *lmath;
	fsg_model_t *fsg;
	jsgf_t *jsgf;
	jsgf_rule_t *rule;

	/* Initialize a logmath object to pass to fsg_model_read */
	lmath = logmath_init(1.0001, 0, 0);
	jsgf = jsgf_parse_file(LMDIR "/polite.gram", NULL);
	rule = jsgf_get_rule(jsgf, "<polite.startPolite>");
	fsg = jsgf_build_fsg(jsgf, rule, lmath);

	jsgf_grammar_free(jsgf);
	fsg_model_free(fsg);
	logmath_free(lmath);

	return 0;
}
