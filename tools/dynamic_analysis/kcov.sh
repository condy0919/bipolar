#!/bin/bash

WORKSPACE=$(dirname $(dirname $(dirname $(realpath "$0"))))

# There must be ${WORKSPACE}/WORKSPACE.
if [ ! -f "${WORKSPACE}/WORKSPACE" ]; then
  echo "File not found: ${WORKSPACE}/WORKSPACE"
  exit 1
fi

export PATH="/opt/kcov/35/bin:${PATH}"

kcov \
    "--include-path=${WORKSPACE}" \
    --verify \
    --exclude-pattern=third_party \
    "${WORKSPACE}/bazel-kcov" \
    "--replace-src-path=/proc/self/cwd:${WORKSPACE}" \
    "$@"
