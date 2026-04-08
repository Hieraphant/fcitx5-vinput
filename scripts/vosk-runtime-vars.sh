#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
# shellcheck source=./vosk-vars.sh
source "${script_dir}/vosk-vars.sh"

VOSK_RUNTIME_ARCH="x86_64"
OPENFST_VOSK_REV="${OPENFST_VOSK_REV:-18e94e63870ebcf79ebb42b7035cd3cb626ec090}"
KALDI_VOSK_REV="${KALDI_VOSK_REV:-bc5baf14231660bd50b7d05788865b4ac6c34481}"

vosk_runtime_set_vars() {
    local target="${1:?target is required}"
    local kaldi_short="${KALDI_VOSK_REV:0:12}"
    local openfst_short="${OPENFST_VOSK_REV:0:12}"

    case "${target}" in
        ubuntu24.04|debian12|opensuse-leap)
            ;;
        *)
            echo "unsupported vosk runtime target: ${target}" >&2
            return 1
            ;;
    esac

    VOSK_RUNTIME_TARGET="${target}"
    VOSK_RUNTIME_BASENAME="vosk-runtime-${target}-${VOSK_RUNTIME_ARCH}-v${VOSK_VERSION}-kaldi-${kaldi_short}-openfst-${openfst_short}"
    VOSK_RUNTIME_ARCHIVE="${VOSK_RUNTIME_BASENAME}.tar.gz"
    VOSK_RUNTIME_MANIFEST="${VOSK_RUNTIME_BASENAME}.manifest.json"
    VOSK_RUNTIME_RELEASE_TAG="vosk-runtime-v${VOSK_VERSION}-kaldi-${kaldi_short}-openfst-${openfst_short}"
}
