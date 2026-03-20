#!/usr/bin/env bash
# Install pre-built sherpa-onnx shared libraries from a local archive or
# directly from the upstream release.
# Usage: build-sherpa-onnx.sh [version] [prefix] [archive_path]
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
# shellcheck source=./sherpa-onnx-vars.sh
source "${script_dir}/sherpa-onnx-vars.sh"

version="${1:-${SHERPA_ONNX_VERSION}}"
prefix="${2:-/usr}"
archive_path="${3:-}"

sherpa_onnx_set_vars "${version}"

workdir="$(mktemp -d)"
trap 'rm -rf "${workdir}"' EXIT

if [[ -z "${archive_path}" ]]; then
    archive_path="${workdir}/${SHERPA_ONNX_ARCHIVE}"
    curl -fL --retry 3 --retry-delay 2 -o "${archive_path}" "${SHERPA_ONNX_URL}"
fi

tar -xjf "${archive_path}" -C "${workdir}"

install -d "${prefix}/lib" "${prefix}/include/sherpa-onnx/c-api"
install -m 755 "${workdir}/${SHERPA_ONNX_STRIP_DIR}/lib/"*.so "${prefix}/lib/"
install -m 644 "${workdir}/${SHERPA_ONNX_STRIP_DIR}/include/sherpa-onnx/c-api/"*.h "${prefix}/include/sherpa-onnx/c-api/"
