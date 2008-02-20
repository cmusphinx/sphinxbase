#!/bin/sh

. ../testfuncs.sh

ulimit -c 0
./test_ckd_alloc_abort
if [ $? = 134 ]; then # 134 = 128 + SIGABRT (as defined in POSIX.1)
    pass expected_failure
else
    fail expected_failure
fi

