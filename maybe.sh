#!/bin/bash

CC="./mcc"

gcc -E -std=c89 -U__GNUC__ -U__GNUC_MINOR__ -U__GNUC_PACKLEVEL__ $1 | \
    $CC -o ./a.ll && \
    llc ./a.ll --filetype=obj -o ./a.o && \
    gcc ./a.o -lm
