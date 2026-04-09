#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
    echo "usage: $0 <linux-x86_64> <runtime-prefix> <output-dir>" >&2
    exit 1
fi

target="$1"
prefix="$2"
output_dir="$3"

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
# shellcheck source=./vosk-runtime-vars.sh
source "${script_dir}/vosk-runtime-vars.sh"

vosk_runtime_set_vars "${target}"

test -f "${prefix}/include/vosk_api.h"
test -f "${prefix}/lib/libvosk.so"

mkdir -p "${output_dir}"

stage_dir="$(mktemp -d)"
trap 'rm -rf "${stage_dir}"' EXIT

mkdir -p "${stage_dir}/include" "${stage_dir}/lib"
install -m 644 "${prefix}/include/vosk_api.h" "${stage_dir}/include/"
install -m 755 "${prefix}/lib/libvosk.so" "${stage_dir}/lib/"

shopt -s nullglob
private_libs=(
    "${prefix}/lib"/libkaldi*.so*
    "${prefix}/lib"/libfst*.so*
)
shopt -u nullglob

if [[ ${#private_libs[@]} -eq 0 ]]; then
    echo "no private kaldi/openfst libraries found under ${prefix}/lib" >&2
    exit 1
fi

install -m 755 "${private_libs[@]}" "${stage_dir}/lib/"

manifest_path="${output_dir}/${VOSK_RUNTIME_MANIFEST}"
archive_path="${output_dir}/${VOSK_RUNTIME_ARCHIVE}"

cat > "${manifest_path}" <<EOF
{
  "target": "${VOSK_RUNTIME_TARGET}",
  "arch": "${VOSK_RUNTIME_ARCH}",
  "vosk_version": "${VOSK_VERSION}",
  "kaldi_rev": "${KALDI_VOSK_REV}",
  "openfst_rev": "${OPENFST_VOSK_REV}",
  "archive": "${VOSK_RUNTIME_ARCHIVE}"
}
EOF

tar -C "${stage_dir}" -czf "${archive_path}" include lib
(
    cd "${output_dir}"
    sha256sum "${VOSK_RUNTIME_ARCHIVE}" > "${VOSK_RUNTIME_ARCHIVE}.sha256"
)
