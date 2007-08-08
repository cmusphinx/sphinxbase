INFO: ../../../src/libsphinxutil/cmd_ln.c(430): Parsing command line:
/home/dhuggins/Projects/Sphinx/sphinxbase/i386-build/test/unit/test_cmdln/.libs/lt-cmdln_parse \
	-a foobar 

Arguments list definition:
[NAME]	[DEFLT]	[DESCR]
-a	42	This is the first argument.
-b		This is the second argument.
-c	no	This is the third argument.
-d	1e-50	This is the fourth argument.

ERROR: "../../../src/libsphinxutil/cmd_ln.c", line 478: Bad argument value for -a: foobar
ERROR: "../../../src/libsphinxutil/cmd_ln.c", line 543: cmd_ln_parse failed, forced exit
