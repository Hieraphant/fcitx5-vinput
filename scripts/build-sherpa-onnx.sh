#!/usr/bin/env bash
# Download and install pre-built sherpa-onnx shared libraries.
# Usage: build-sherpa-onnx.sh [version] [prefix]
set -euo pipefail

version="${1:-1.12.28}"
prefix="${2:-/usr}"

archive="sherpa-onnx-v${version}-linux-x64-shared-no-tts.tar.bz2"
url="https://github.com/k2-fsa/sherpa-onnx/releases/download/v${version}/${archive}"
strip_dir="sherpa-onnx-v${version}-linux-x64-shared-no-tts"

workdir="$(mktemp -d)"
trap 'rm -rf "${workdir}"' EXIT

curl -sL "${url}" | tar -xjf - -C "${workdir}"

install -d "${prefix}/lib" "${prefix}/include/sherpa-onnx/c-api"
install -m 755 "${workdir}/${strip_dir}/lib/"*.so "${prefix}/lib/"
install -m 644 "${workdir}/${strip_dir}/include/sherpa-onnx/c-api/"*.h "${prefix}/include/sherpa-onnx/c-api/"
