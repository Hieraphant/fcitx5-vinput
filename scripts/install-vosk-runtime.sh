#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
    echo "usage: $0 <linux-x86_64> <prefix>" >&2
    exit 1
fi

target="$1"
prefix="$2"

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
# shellcheck source=./vosk-runtime-vars.sh
source "${script_dir}/vosk-runtime-vars.sh"

vosk_runtime_set_vars "${target}"

repo="${VOSK_RUNTIME_REPO:-xifan2333/fcitx5-vinput}"
base_url="https://github.com/${repo}/releases/download/${VOSK_RUNTIME_RELEASE_TAG}"

workdir="$(mktemp -d)"
trap 'rm -rf "${workdir}"' EXIT

archive_path="${workdir}/${VOSK_RUNTIME_ARCHIVE}"
sha256_path="${archive_path}.sha256"
manifest_path="${workdir}/${VOSK_RUNTIME_MANIFEST}"

curl -fL --retry 3 --retry-delay 2 \
    -o "${archive_path}" \
    "${base_url}/${VOSK_RUNTIME_ARCHIVE}"
curl -fL --retry 3 --retry-delay 2 \
    -o "${sha256_path}" \
    "${base_url}/${VOSK_RUNTIME_ARCHIVE}.sha256"
curl -fL --retry 3 --retry-delay 2 \
    -o "${manifest_path}" \
    "${base_url}/${VOSK_RUNTIME_MANIFEST}"

expected_sha256="$(awk '{print $1}' "${sha256_path}")"
actual_sha256="$(sha256sum "${archive_path}" | awk '{print $1}')"
if [[ -z "${expected_sha256}" || "${actual_sha256}" != "${expected_sha256}" ]]; then
    echo "runtime archive checksum mismatch for ${VOSK_RUNTIME_ARCHIVE}" >&2
    exit 1
fi

if ! grep -Fq "\"target\": \"${VOSK_RUNTIME_TARGET}\"" "${manifest_path}"; then
    echo "runtime manifest target mismatch for ${target}" >&2
    exit 1
fi

mkdir -p "${prefix}"
tar -C "${prefix}" -xzf "${archive_path}"

test -f "${prefix}/include/vosk_api.h"
test -f "${prefix}/lib/libvosk.so"
