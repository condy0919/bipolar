#!/bin/bash

mydir=$(dirname $(realpath "$0"))

export TSAN_OPTIONS="$TSAN_OPTIONS:detect_deadlocks=1:second_deadlock_stack=1:strip_path_prefix=/proc/self/cwd/:suppressions=$mydir/tsan.supp"

"$@"
