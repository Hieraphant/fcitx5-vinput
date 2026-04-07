#!/usr/bin/env bash
set -euo pipefail

script_path="${1:-scripts/build-vosk-from-source.sh}"

if ! grep -Fq 'cmake -S "${kaldi_src}" -B "${kaldi_build}"' "${script_path}"; then
    echo "Expected ${script_path} to configure Kaldi from the repo root." >&2
    exit 1
fi
