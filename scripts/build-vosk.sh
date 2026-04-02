#!/usr/bin/env bash
# Install pre-built vosk shared library from a local archive or
# from the upstream release after checksum verification.
# Usage: build-vosk.sh [version] [prefix] [archive_path]
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
# shellcheck source=./vosk-vars.sh
source "${script_dir}/vosk-vars.sh"

version="${1:-${VOSK_VERSION}}"
prefix="${2:-/usr}"
archive_path="${3:-}"

vosk_set_vars "${version}"

workdir="$(mktemp -d)"
trap 'rm -rf "${workdir}"' EXIT

if [[ -z "${archive_path}" ]]; then
    archive_path="${workdir}/${VOSK_ARCHIVE}"
    curl -fL --retry 3 --retry-delay 2 -o "${archive_path}" "${VOSK_URL}"
fi

expected_sha256="${VOSK_SHA256:-}"
if [[ -z "${expected_sha256}" ]]; then
    echo "Fetching digest from GitHub API..." >&2
    expected_sha256="$(vosk_fetch_digest "${version}" "${VOSK_ARCHIVE}")" || {
        echo "Failed to fetch digest for ${VOSK_ARCHIVE} from GitHub API." >&2
        exit 1
    }
fi

actual_sha256="$(sha256sum "${archive_path}" | awk '{print $1}')"
if [[ "${actual_sha256}" != "${expected_sha256}" ]]; then
    echo "Checksum mismatch for ${archive_path}: expected ${expected_sha256}, got ${actual_sha256}" >&2
    exit 1
fi

unzip -q "${archive_path}" -d "${workdir}"

install -d "${prefix}/lib" "${prefix}/include"
install -m 755 "${workdir}/${VOSK_STRIP_DIR}/libvosk.so" "${prefix}/lib/"
install -m 644 "${workdir}/${VOSK_STRIP_DIR}/vosk_api.h" "${prefix}/include/"
