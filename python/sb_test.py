#!/usr/bin/env python

import sphinxbase
import sys

lm = sphinxbase.NGramModel(sys.argv[1])
for ng in lm.mgrams(3):
    print ng.log_prob, ng.log_bowt
