#!/usr/bin/env bash

sherpa_onnx_set_vars() {
    local version="${1:?version is required}"

    SHERPA_ONNX_VERSION="${version}"
    SHERPA_ONNX_ARCHIVE="sherpa-onnx-v${version}-linux-x64-shared-no-tts.tar.bz2"
    SHERPA_ONNX_STRIP_DIR="sherpa-onnx-v${version}-linux-x64-shared-no-tts"
    SHERPA_ONNX_URL="https://github.com/k2-fsa/sherpa-onnx/releases/download/v${version}/${SHERPA_ONNX_ARCHIVE}"
    SHERPA_ONNX_SHA256="${SHERPA_ONNX_SHA256:-}"

    case "${version}" in
        1.12.28)
            if [[ -z "${SHERPA_ONNX_SHA256}" ]]; then
                SHERPA_ONNX_SHA256="4f1ad4ba5c80271f55b1ec6f0a93083aa1588a86ab1491153e48064f4ed3870b"
            fi
            ;;
    esac
}

sherpa_onnx_set_vars "${SHERPA_ONNX_VERSION:-1.12.28}"
