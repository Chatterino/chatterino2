#!/bin/bash

set -ev;

# aqt installs into .qtinstall/Qt/<version>/gcc_64
# This is doing the same as jurplel/install-qt-action
# See https://github.com/jurplel/install-qt-action/blob/74ca8cd6681420fc8894aed264644c7a76d7c8cb/action/src/main.ts#L52-L74
qtpath=$(echo .qtinstall/Qt/[0-9]*/*/bin/qmake | sed -e s:/bin/qmake$::)
export LD_LIBRARY_PATH="$qtpath/lib"
export QT_ROOT_DIR=$qtpath
export QT_PLUGIN_PATH="$qtpath/plugins"
export PATH="$PATH:$(realpath "$qtpath/bin")"
export Qt6_DIR="$(realpath "$qtpath")"

cmake -S. -Bbuild-clang-tidy \
    -DCMAKE_BUILD_TYPE=Debug \
    -DPAJLADA_SETTINGS_USE_BOOST_FILESYSTEM=On \
    -DUSE_PRECOMPILED_HEADERS=OFF \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=On \
    -DCHATTERINO_LTO=Off \
    -DCHATTERINO_PLUGINS=On \
    -DBUILD_TESTS=On \
    -DBUILD_BENCHMARKS=On

# Run MOC and UIC
# This will compile the dependencies
# Get the targets using `ninja -t targets | grep autogen`
cmake --build build-clang-tidy --parallel -t \
    Core_autogen \
    LibCommuni_autogen \
    Model_autogen \
    Util_autogen \
    chatterino-lib_autogen
