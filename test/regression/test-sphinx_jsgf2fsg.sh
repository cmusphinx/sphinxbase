#!/bin/sh
. ./testfuncs.sh

echo "JSGF2FSG TEST"
rules="test.rightRecursion test.nestedRightRecursion test.kleene test.nulltest test.command"

tmpout="test-jsgf2fsg.out"
rm -f $tmpout

export JSGF_PATH=$tests/regression
for r in $rules; do
    run_program sphinx_jsgf2fsg $tests/regression/test.gram $r > $r.out 2>>$tmpout
    compare_table $r $r.out $tests/regression/$r.fsg
done

