#!/bin/bash

mydir=$(dirname $(realpath "$0"))

export UBSAN_OPTIONS="$UBSAN_OPTIONS:halt_on_error=1:strip_path_prefix=/proc/self/cwd/:suppressions=$mydir/ubsan.supp"

"$@"
