#!/bin/sh

rm -f config.h
echo "Building for Linux"

set -e

ARCH="aarch64"

echo -e "\033[1;31m Warning:\033[0;31m 先设置链接库路径CUSTOM_LINK_PATH."
CUSTOM_LINK_PATH="/usr/lib-flip"

GENERAL="
    --disable-shared \
    --enable-static"

MODULES="\
    --disable-avdevice \
    --disable-filters \
    --disable-programs \
    --disable-network \
    --disable-avfilter \
    --disable-postproc \
    --disable-encoders \
    --disable-doc \
    --disable-ffplay \
    --disable-ffprobe \
    --disable-ffmpeg"

VIDEO_DECODERS="\
    --enable-decoder=h264 \
    --enable-decoder=mpeg4 \
    --enable-decoder=h263 \
    --enable-decoder=h263p \
    --enable-decoder=mpeg2video \
    --enable-decoder=mjpeg \
    --enable-decoder=mjpegb"

AUDIO_DECODERS="\
    --enable-decoder=aac \
    --enable-decoder=aac_latm \
    --enable-decoder=wavpack \
    --enable-decoder=amrnb \
    --enable-decoder=amrwb \
    --enable-decoder=amr \
    --enable-decoder=mp3 \
    --enable-decoder=pcm_s16le \
    --enable-decoder=pcm_s8"

DEMUXERS="\
    --enable-demuxer=h264 \
    --enable-demuxer=m4v \
    --enable-demuxer=mp3 \
    --enable-demuxer=mpegvideo \
    --enable-demuxer=mpegps \
    --enable-demuxer=mjpeg \
    --enable-demuxer=mov \
    --enable-demuxer=avi \
    --enable-demuxer=aac \
    --enable-demuxer=amr \
    --enable-demuxer=amrnb \
    --enable-demuxer=amrwb \
    --enable-demuxer=pcm_s16le \
    --enable-demuxer=pcm_s8 \
    --enable-demuxer=wav"

VIDEO_ENCODERS=""

AUDIO_ENCODERS="\
    --enable-encoder=pcm_s16le"

MUXERS="\
    --enable-muxer=amr \
    --enable-muxer=avi \
    --enable-muxer=mp3 \
    --enable-muxer=wav \
    --enable-muxer=pcm_s16le \
    --enable-muxer=pcm_s8 \
    --enable-muxer=ogg"

PARSERS="\
    --enable-parser=h264 \
    --enable-parser=mpeg4video \
    --enable-parser=mpegvideo \
    --enable-parser=aac \
    --enable-parser=aac_latm \
    --enable-parser=mpegaudio"

PROTOCOLS="\
    --enable-protocol=file"

./configure \
    --prefix=./linux/${ARCH} \
    ${GENERAL} \
    --extra-cflags="-D__STDC_CONSTANT_MACROS -O3" \
    --enable-zlib \
    --enable-pic \
    --disable-yasm \
    --disable-everything \
    ${MODULES} \
    ${VIDEO_DECODERS} \
    ${AUDIO_DECODERS} \
    ${VIDEO_ENCODERS} \
    ${AUDIO_ENCODERS} \
    ${DEMUXERS} \
    ${MUXERS} \
    ${PARSERS} \
    ${PROTOCOLS} \
    --arch=${ARCH} \
    --cc=aarch64-none-linux-gnu-gcc \
    --cxx=aarch64-none-linux-gnu-g++ \
    --enable-cross-compile \
    --extra-ldflags="-L${CUSTOM_LINK_PATH} -Wl,-rpath-link=${CUSTOM_LINK_PATH}"


make clean
make install
