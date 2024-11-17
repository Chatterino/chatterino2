#!/bin/sh

# TODO: Investigate if the -fno-sized-deallocation flag is still necessary
# TODO: Test appimage/deb creation

set -e

env

BUILD_TESTS="On"
BUILD_BENCHMARKS="ON"

ubuntu_version="$(lsb_release -sr)"
if [ "$ubuntu_version" = "20.04" ]; then
    BUILD_TESTS="Off"
    BUILD_BENCHMARKS="Off"
fi

rm -rf build
mkdir build
cmake \
    -B build \
    -DCMAKE_INSTALL_PREFIX=appdir/usr/ \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_APP=On \
    -DBUILD_TESTS="$BUILD_TESTS" \
    -DBUILD_BENCHMARKS="$BUILD_BENCHMARKS" \
    -DUSE_PRECOMPILED_HEADERS=OFF \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=On \
    -DCHATTERINO_PLUGINS="$C2_PLUGINS" \
    -DCMAKE_PREFIX_PATH="$Qt6_DIR/lib/cmake" \
    -DCHATTERINO_STATIC_QT_BUILD=On \
    -DCMAKE_CXX_FLAGS="-fno-sized-deallocation" \
    .
cmake --build build

# sh ./../.CI/CreateAppImage.sh
# sh ./../.CI/CreateUbuntuDeb.sh
