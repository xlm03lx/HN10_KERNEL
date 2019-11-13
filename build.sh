#!/bin/bash
export PATH=$PATH:/home/acuicultor/Android/toolchains/aarch64-linux-android-4.9/bin
export CROSS_COMPILE=aarch64-linux-android-
mkdir out
make ARCH=arm64 O=out radioactive_defconfig
make ARCH=arm64 O=out -j5

