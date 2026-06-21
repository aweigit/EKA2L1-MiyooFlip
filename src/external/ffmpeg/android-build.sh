#!/bin/bash

NDK_HOME=$HOME/sdk/ndk-bundle
PLATFORM=linux
TOOLCHAIN=${NDK_HOME}/toolchains/llvm/prebuilt/${PLATFORM}-x86_64
ANDROID_API=21

echo PLATFORM=${PLATFORM}
echo NDK_HOME=${NDK_HOME}
echo TOOLCHAIN=${TOOLCHAIN}
echo ANDROID_API=${ANDROID_API}

get_cpu_count() {
if [ "$(uname)" == "Darwin" ]; then
    echo $(sysctl -n hw.physicalcpu)
else
    echo $(nproc)
fi
}

build_ffmpeg() {
echo "Compiling FFmpeg for $CPU"
./configure \
    --prefix=${PREFIX} \
    --pkg-config=pkg-config \
    ${ASM_FLAGS} \
    --disable-everything \
    --enable-cross-compile \
    --enable-pic \
    --disable-shared \
    --enable-static \
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
    --disable-ffmpeg \
    ${SIZE_OPTIONS} \
    ${DEBUG_OPTIONS} \
    --enable-decoder=h264 \
    --enable-decoder=mpeg4 \
    --enable-decoder=h263 \
    --enable-decoder=h263p \
    --enable-decoder=mpeg2video \
    --enable-decoder=mjpeg \
    --enable-decoder=mjpegb \
    --enable-decoder=aac \
    --enable-decoder=aac_latm \
    --enable-decoder=wavpack \
    --enable-decoder=amrnb \
    --enable-decoder=amrwb \
    --enable-decoder=amr \
    --enable-decoder=mp3 \
    --enable-decoder=pcm_s16le \
    --enable-decoder=pcm_s8 \
    --enable-demuxer=h264 \
    --enable-demuxer=m4v \
    --enable-demuxer=mp3 \
    --enable-demuxer=mov \
    --enable-demuxer=mpegvideo \
    --enable-demuxer=mpegps \
    --enable-demuxer=mjpeg \
    --enable-demuxer=mov \
    --enable-demuxer=avi \
    --enable-demuxer=aac \
    --enable-demuxer=pcm_s16le \
    --enable-demuxer=pcm_s8 \
    --enable-demuxer=wav \
    --enable-demuxer=amr \
    --enable-demuxer=amrnb \
    --enable-demuxer=amrwb \
    --enable-encoder=pcm_s16le \
    --enable-muxer=amr \
    --enable-muxer=avi \
    --enable-muxer=mp3 \
    --enable-muxer=wav \
    --enable-muxer=pcm_s16le \
    --enable-muxer=pcm_s8 \
    --enable-muxer=ogg \
    --enable-parser=h264 \
    --enable-parser=mpeg4video \
    --enable-parser=mpegvideo \
    --enable-parser=aac \
    --enable-parser=aac_latm \
    --enable-parser=mpegaudio \
    --enable-protocol=file \
    --cross-prefix=${CROSS_PREFIX} \
    --target-os=android \
    --arch=${ARCH} \
    --cpu=${CPU} \
    --cc=${CC} \
    --cxx=${CXX} \
    --enable-cross-compile \
    --sysroot=${SYSROOT} \
    --extra-cflags="-Os -fpic -DVK_ENABLE_BETA_EXTENSIONS=0 ${OPTIMIZE_CFLAGS}" \
    --extra-ldflags="${ADDI_LDFLAGS}" \
    ${ADDITIONAL_CONFIGURE_FLAG}
make clean
make -j$(get_cpu_count)
make install
echo "The Compilation of FFmpeg for $CPU is completed"
}

#armv7
ARCH=arm
CPU=armv7-a
CC=${TOOLCHAIN}/bin/armv7a-linux-androideabi${ANDROID_API}-clang
CXX=${TOOLCHAIN}/bin/armv7a-linux-androideabi${ANDROID_API}-clang++
SYSROOT=${NDK_HOME}/toolchains/llvm/prebuilt/${PLATFORM}-x86_64/sysroot
CROSS_PREFIX=${TOOLCHAIN}/bin/llvm-
PREFIX=$(pwd)/android/armeabi-v7a
OPTIMIZE_CFLAGS="-march=${CPU}"
ASM_FLAGS=" --enable-neon --enable-asm --enable-inline-asm"
build_ffmpeg

#armv8-a
ARCH=arm64
CPU=armv8-a
CC=${TOOLCHAIN}/bin/aarch64-linux-android${ANDROID_API}-clang
CXX=${TOOLCHAIN}/bin/aarch64-linux-android${ANDROID_API}-clang++
SYSROOT=${NDK_HOME}/toolchains/llvm/prebuilt/${PLATFORM}-x86_64/sysroot
CROSS_PREFIX=${TOOLCHAIN}/bin/llvm-
PREFIX=$(pwd)/android/arm64-v8a
OPTIMIZE_CFLAGS="-march=${CPU}"
ASM_FLAGS=" --enable-neon --enable-asm --enable-inline-asm"
build_ffmpeg

#x86_64
ARCH=x86_64
CPU=x86-64
CC=${TOOLCHAIN}/bin/x86_64-linux-android${ANDROID_API}-clang
CXX=${TOOLCHAIN}/bin/x86_64-linux-android${ANDROID_API}-clang++
SYSROOT=${NDK_HOME}/toolchains/llvm/prebuilt/${PLATFORM}-x86_64/sysroot
CROSS_PREFIX=${TOOLCHAIN}/bin/llvm-
PREFIX=$(pwd)/android/x86_64
OPTIMIZE_CFLAGS="-march=$CPU -msse4.2 -mpopcnt -m64"
ASM_FLAGS=" --disable-neon --disable-asm --disable-inline-asm"
build_ffmpeg
