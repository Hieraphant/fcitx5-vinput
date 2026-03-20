#!/usr/bin/env bash

sherpa_onnx_set_vars() {
    local version="${1:?version is required}"

    SHERPA_ONNX_VERSION="${version}"
    SHERPA_ONNX_ARCHIVE="sherpa-onnx-v${version}-linux-x64-shared-no-tts.tar.bz2"
    SHERPA_ONNX_STRIP_DIR="sherpa-onnx-v${version}-linux-x64-shared-no-tts"
    SHERPA_ONNX_URL="https://github.com/k2-fsa/sherpa-onnx/releases/download/v${version}/${SHERPA_ONNX_ARCHIVE}"
}

sherpa_onnx_set_vars "${SHERPA_ONNX_VERSION:-1.12.28}"
