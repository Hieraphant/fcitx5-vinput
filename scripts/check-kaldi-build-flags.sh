#!/usr/bin/env bash
set -euo pipefail

script_path="${1:-scripts/build-vosk-from-source.sh}"

if ! grep -Fq -- '-DKALDI_BUILD_TEST=OFF' "${script_path}"; then
    echo "Expected ${script_path} to disable Kaldi test targets." >&2
    exit 1
fi
