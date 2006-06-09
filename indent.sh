#!/bin/sh

# This is the equivalent of the following Emacs thing:
# -*- tab-width: 8; c-file-style: "linux" -*-

find . -name '*.c' -print0 | xargs indent -i8 -kr -psl -nce -nut
