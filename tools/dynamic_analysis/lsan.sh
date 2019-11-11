#!/bin/bash

mydir=$(dirname $(realpath "$0"))

export LSAN_OPTIONS="$LSAN_OPTIONS:strip_path_prefix=/proc/self/cwd/:suppressions=$mydir/lsan.supp"

"$@"
