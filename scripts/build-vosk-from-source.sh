#!/usr/bin/env bash
# Build libvosk.so from source in the current environment so the bundled
# runtime matches the target distro ABI.
# Usage: build-vosk-from-source.sh [version] [prefix]
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
# shellcheck source=./vosk-vars.sh
source "${script_dir}/vosk-vars.sh"

version="${1:-${VOSK_VERSION}}"
prefix="${2:-/usr}"

openfst_rev="${OPENFST_VOSK_REV:-18e94e63870ebcf79ebb42b7035cd3cb626ec090}"
kaldi_rev="${KALDI_VOSK_REV:-bc5baf14231660bd50b7d05788865b4ac6c34481}"
jobs="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)}"
cc="${CC:-clang}"
cxx="${CXX:-clang++}"

# Kaldi's CMake expects BLAS_LIBRARIES/LAPACK_LIBRARIES to be populated but
# does not call find_package(BLAS) in the non-CONDA OpenBLAS path.  Detect
# them here so the linker flags are never empty.
if [ -z "${BLAS_LIBRARIES:-}" ]; then
    for _candidate in /usr/lib64/libopenblas.so /usr/lib/x86_64-linux-gnu/libopenblas.so /usr/lib/libopenblas.so; do
        if [ -f "${_candidate}" ]; then
            BLAS_LIBRARIES="${_candidate}"
            break
        fi
    done
    : "${BLAS_LIBRARIES:=-lopenblas}"
fi
if [ -z "${LAPACK_LIBRARIES:-}" ]; then
    for _candidate in /usr/lib64/liblapack.so /usr/lib/x86_64-linux-gnu/liblapack.so /usr/lib/liblapack.so; do
        if [ -f "${_candidate}" ]; then
            LAPACK_LIBRARIES="${_candidate}"
            break
        fi
    done
    : "${LAPACK_LIBRARIES:=-llapack}"
fi

workdir="$(mktemp -d)"
trap 'rm -rf "${workdir}"' EXIT

openfst_src="${workdir}/openfst"
kaldi_src="${workdir}/kaldi"
kaldi_build="${workdir}/kaldi-build"
vosk_src="${workdir}/vosk-api"
patch_file="${workdir}/kaldi-openfst-ngram.patch"

git clone --filter=blob:none --no-checkout https://github.com/alphacep/openfst.git "${openfst_src}"
git -C "${openfst_src}" checkout --detach "${openfst_rev}"

git clone --filter=blob:none --no-checkout https://github.com/alphacep/kaldi.git "${kaldi_src}"
git -C "${kaldi_src}" checkout --detach "${kaldi_rev}"

sed "s|@OPENFST_ROOT@|${openfst_src}|g" \
    "${script_dir}/../patches/kaldi-openfst-ngram.patch" > "${patch_file}"
patch -d "${kaldi_src}" -p0 < "${patch_file}"

cmake -S "${kaldi_src}" -B "${kaldi_build}" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${prefix}" \
    -DCMAKE_INSTALL_LIBDIR=lib \
    -DBUILD_SHARED_LIBS=ON \
    -DKALDI_BUILD_TEST=OFF \
    -DFETCHCONTENT_SOURCE_DIR_OPENFST:PATH="${openfst_src}" \
    -DCMAKE_C_COMPILER="${cc}" \
    -DCMAKE_CXX_COMPILER="${cxx}" \
    -DBLAS_LIBRARIES="${BLAS_LIBRARIES}" \
    -DLAPACK_LIBRARIES="${LAPACK_LIBRARIES}" \
    -DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=mold \
    -DCMAKE_SHARED_LINKER_FLAGS=-fuse-ld=mold \
    -DCMAKE_C_FLAGS="-I${openfst_src}/src/include" \
    -DCMAKE_CXX_FLAGS="-I${openfst_src}/src/include"
cmake --build "${kaldi_build}" --parallel "${jobs}"
cmake --install "${kaldi_build}"

# Vosk's upstream Makefile expects a KALDI_ROOT/libs directory when linking
# against shared Kaldi/OpenFST libraries.
ln -sfn "${prefix}/lib" "${prefix}/libs"

git clone --branch "v${version}" --depth 1 https://github.com/alphacep/vosk-api.git "${vosk_src}"

make -C "${vosk_src}/src" -j"${jobs}" \
    KALDI_ROOT="${prefix}" \
    OPENFST_ROOT="${prefix}" \
    USE_SHARED=1 \
    HAVE_OPENBLAS_CLAPACK=0 \
    CXX="${cxx}" \
    EXTRA_CFLAGS="-I${prefix}/include/kaldi -I${prefix}/include/openfst" \
    EXTRA_LDFLAGS="-L${prefix}/lib -Wl,-rpath,${prefix}/lib -fuse-ld=mold"

install -d "${prefix}/lib" "${prefix}/include"
install -m 755 "${vosk_src}/src/libvosk.so" "${prefix}/lib/"
install -m 644 "${vosk_src}/src/vosk_api.h" "${prefix}/include/"
