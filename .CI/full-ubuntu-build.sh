#!/bin/sh

set -e

mkdir build
cmake -B build
CXXFLAGS=-fno-sized-deallocation cmake \
    -B build \
    -DCMAKE_INSTALL_PREFIX=appdir/usr/ \
    -DCMAKE_BUILD_TYPE=Release \
    -DUSE_PRECOMPILED_HEADERS=OFF \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=On \
    -DCHATTERINO_PLUGINS="$C2_PLUGINS" \
    -DCMAKE_PREFIX_PATH="$Qt6_DIR/lib/cmake" \
    -DBUILD_WITH_QT6="$C2_BUILD_WITH_QT6" \
    ..
cmake --build build

# TODO: Test appimage/deb creation
# sh ./../.CI/CreateAppImage.sh
# sh ./../.CI/CreateUbuntuDeb.sh
