#!/bin/sh

# TODO: Investigate if the -fno-sized-deallocation flag is still necessary
# TODO: Test appimage/deb creation

set -e

env

rm -rf build
mkdir build
cmake \
    -B build \
    -DCMAKE_INSTALL_PREFIX=appdir/usr/ \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_APP=On \
    -DBUILD_TESTS=On \
    -DBUILD_BENCHMARKS=On \
    -DUSE_PRECOMPILED_HEADERS=OFF \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=On \
    -DCHATTERINO_PLUGINS="$C2_PLUGINS" \
    -DCMAKE_PREFIX_PATH="$Qt6_DIR/lib/cmake" \
    -DBUILD_WITH_QT6="$C2_BUILD_WITH_QT6" \
    -DCHATTERINO_STATIC_QT_BUILD=On \
    -DCMAKE_CXX_FLAGS="-fno-sized-deallocation" \
    .
cmake --build build

# sh ./../.CI/CreateAppImage.sh
# sh ./../.CI/CreateUbuntuDeb.sh
