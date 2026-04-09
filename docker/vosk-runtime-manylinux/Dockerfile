FROM quay.io/pypa/manylinux2014_x86_64

LABEL description="Build image for generic Linux Vosk runtime assets"

ARG KALDI_VOSK_REV=bc5baf14231660bd50b7d05788865b4ac6c34481
ARG OPENFST_VOSK_REV=18e94e63870ebcf79ebb42b7035cd3cb626ec090

ENV PATH=/opt/rh/devtoolset-8/root/usr/bin:${PATH}
ENV LD_LIBRARY_PATH=/opt/rh/devtoolset-8/root/usr/lib64:${LD_LIBRARY_PATH}

RUN yum -y update && yum -y install \
    devtoolset-8-libatomic-devel \
    autoconf \
    automake \
    cmake \
    git \
    libffi-devel \
    libtool \
    make \
    patch \
    && yum clean all

RUN cd /opt \
    && git clone https://github.com/alphacep/kaldi kaldi \
    && git -C /opt/kaldi checkout --detach "${KALDI_VOSK_REV}" \
    && cd /opt/kaldi/tools \
    && git clone -b v0.3.20 --single-branch https://github.com/xianyi/OpenBLAS \
    && git clone -b v3.2.1 --single-branch https://github.com/alphacep/clapack \
    && make -C OpenBLAS ONLY_CBLAS=1 DYNAMIC_ARCH=1 TARGET=NEHALEM USE_LOCKING=1 USE_THREAD=0 all \
    && make -C OpenBLAS PREFIX=$(pwd)/OpenBLAS/install install \
    && mkdir -p clapack/BUILD \
    && cd clapack/BUILD \
    && cmake .. \
    && make -j"$(nproc)" \
    && find . -name "*.a" -exec cp {} ../../OpenBLAS/install/lib/ \; \
    && cd /opt/kaldi/tools \
    && git clone https://github.com/alphacep/openfst openfst-src \
    && git -C openfst-src checkout --detach "${OPENFST_VOSK_REV}" \
    && cd openfst-src \
    && autoreconf -i \
    && CFLAGS="-g -O3" ./configure \
        --prefix=/opt/kaldi/tools/openfst \
        --enable-static \
        --enable-shared \
        --enable-far \
        --enable-ngram-fsts \
        --enable-lookahead-fsts \
        --with-pic \
        --disable-bin \
    && make -j"$(nproc)" \
    && make install \
    && cd /opt/kaldi/src \
    && ./configure --mathlib=OPENBLAS_CLAPACK --shared --use-cuda=no \
    && sed -i 's:-msse -msse2:-msse -msse2:g' kaldi.mk \
    && sed -i 's: -O1 : -O3 :g' kaldi.mk \
    && make -j"$(nproc)" online2 rnnlm \
    && find /opt/kaldi -name "*.o" -exec rm {} \;
