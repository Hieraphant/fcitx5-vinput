#!/usr/bin/env bash
# Install pre-built sherpa-onnx shared libraries.
# Usage: build-sherpa-onnx.sh [version] [prefix] [archive_path]
set -euo pipefail

version="${1:-1.12.28}"
prefix="${2:-/usr}"
archive_path="${3:-}"

archive="sherpa-onnx-v${version}-linux-x64-shared-no-tts.tar.bz2"
url="https://github.com/k2-fsa/sherpa-onnx/releases/download/v${version}/${archive}"
strip_dir="sherpa-onnx-v${version}-linux-x64-shared-no-tts"

workdir="$(mktemp -d)"
trap 'rm -rf "${workdir}"' EXIT

if [[ -z "${archive_path}" ]]; then
    archive_path="${workdir}/${archive}"
    curl -fL --retry 3 --retry-delay 2 -o "${archive_path}" "${url}"
fi

tar -xjf "${archive_path}" -C "${workdir}"

install -d "${prefix}/lib" "${prefix}/include/sherpa-onnx/c-api"
install -m 755 "${workdir}/${strip_dir}/lib/"*.so "${prefix}/lib/"
install -m 644 "${workdir}/${strip_dir}/include/sherpa-onnx/c-api/"*.h "${prefix}/include/sherpa-onnx/c-api/"
