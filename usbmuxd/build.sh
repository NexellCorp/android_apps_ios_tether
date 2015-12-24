#!/bin/sh

ROOT_DIR=$(pwd)
export ANDROID_NDK_ROOT=/home/pjsin/devel/tools/toolchain/android-ndk/android-ndk-r10e
export TOOLCHAIN=arm-linux-androideabi
export ANDROID_API=android-19
export ANDROID_ARCH=arm

OUT_DIR=$ROOT_DIR/out
mkdir -p $OUT_DIR
TOOLCHAIN_DIR=$OUT_DIR/toolchain
mkdir -p $TOOLCHAIN_DIR
INSTALL_DIR=$OUT_DIR/fsroot
mkdir -p $INSTALL_DIR

/home/pjsin/devel/tools/toolchain/android-ndk/android-ndk-r10e/build/tools/make-standalone-toolchain.sh --platform=android-19 --stl=gnustl --toolchain=arm-linux-androideabi-4.8 --arch=arm --install-dir=$TOOLCHAIN_DIR
./autogen.sh --host=arm-linux-androideabi --with-sysroot=$ANDROID_SYSROOT --prefix=$INSTALL_DIR --without-cython


