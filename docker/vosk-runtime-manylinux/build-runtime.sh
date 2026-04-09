#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "usage: $0 <output-prefix>" >&2
    exit 1
fi

prefix="$1"
repo_root="/io"

# shellcheck source=/dev/null
source "${repo_root}/scripts/vosk-runtime-vars.sh"

jobs="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)}"
workdir="$(mktemp -d)"
trap 'rm -rf "${workdir}"' EXIT

vosk_src="${workdir}/vosk-api"

git clone --branch "v${VOSK_VERSION}" --depth 1 https://github.com/alphacep/vosk-api.git "${vosk_src}"

make -C "${vosk_src}/src" -j"${jobs}" \
    KALDI_ROOT="/opt/kaldi" \
    OPENFST_ROOT="/opt/kaldi/tools/openfst" \
    HAVE_OPENBLAS_CLAPACK=1 \
    USE_SHARED=1 \
    EXTRA_CFLAGS="-I/opt/kaldi/src -I/opt/kaldi/tools/openfst/include -I/opt/kaldi/tools/OpenBLAS/install/include" \
    EXTRA_LDFLAGS="-L/opt/kaldi/src/lib -L/opt/kaldi/tools/openfst/lib -L/opt/kaldi/tools/OpenBLAS/install/lib -Wl,-rpath,/opt/kaldi/src/lib -Wl,-rpath,/opt/kaldi/tools/openfst/lib -Wl,-rpath,/opt/kaldi/tools/OpenBLAS/install/lib"

install -d "${prefix}/include" "${prefix}/lib"
install -m 644 "${vosk_src}/src/vosk_api.h" "${prefix}/include/"
install -m 755 "${vosk_src}/src/libvosk.so" "${prefix}/lib/"
install -m 755 /opt/kaldi/src/lib/libkaldi*.so* "${prefix}/lib/"
install -m 755 /opt/kaldi/tools/openfst/lib/libfst*.so* "${prefix}/lib/"
