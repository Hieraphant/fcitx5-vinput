#!/usr/bin/env bash
set -euo pipefail

patch_file="${1:-patches/kaldi-openfst-ngram.patch}"

mapfile -t headers < <(grep -E '^(---|\+\+\+) ' "${patch_file}")

expected=(
    "--- CMakeLists.txt"
    "+++ CMakeLists.txt"
    "--- cmake/third_party/openfst_lib_target.cmake"
    "+++ cmake/third_party/openfst_lib_target.cmake"
)

if [[ ${#headers[@]} -ne ${#expected[@]} ]]; then
    echo "Unexpected patch header count in ${patch_file}: ${#headers[@]}" >&2
    printf 'Actual headers:\n' >&2
    printf '  %s\n' "${headers[@]}" >&2
    exit 1
fi

for i in "${!expected[@]}"; do
    if [[ "${headers[i]}" != "${expected[i]}"* ]]; then
        echo "Unexpected patch header in ${patch_file} at index ${i}:" >&2
        echo "  expected prefix: ${expected[i]}" >&2
        echo "  actual: ${headers[i]}" >&2
        exit 1
    fi
done
