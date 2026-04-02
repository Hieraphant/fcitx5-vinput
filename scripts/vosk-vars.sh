#!/usr/bin/env bash

VOSK_REPO="alphacep/vosk-api"

vosk_set_vars() {
    local version="${1:?version is required}"

    VOSK_VERSION="${version}"
    VOSK_ARCHIVE="vosk-linux-x86_64-${version}.zip"
    VOSK_STRIP_DIR="vosk-linux-x86_64-${version}"
    VOSK_URL="https://github.com/${VOSK_REPO}/releases/download/v${version}/${VOSK_ARCHIVE}"
    VOSK_SHA256=""

    case "${version}" in
        0.3.45)
            VOSK_SHA256="bbdc8ed85c43979f6443142889770ea95cbfbc56cffb5c5dcd73afa875c5fbb2"
            ;;
    esac
}

vosk_fetch_digest() {
    local version="${1:?version is required}"
    local asset_name="${2:?asset_name is required}"
    local api_url="https://api.github.com/repos/${VOSK_REPO}/releases/tags/v${version}"

    curl -fsSL "${api_url}" \
        | jq -re --arg name "${asset_name}" \
            '.assets[] | select(.name == $name) | .digest // empty
             | ltrimstr("sha256:")'
}

vosk_set_vars "${VOSK_VERSION:-0.3.45}"
