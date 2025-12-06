#!/bin/bash

set -ev;

CLANG_TIDY_VERSION=21
CLAZY_VERSION=v1.16
CLAZY_CLANG_TIDY_HASH=6ef467f518304955ef031fb74e74fa16349361919172c7f0fb622dcf4b4b7030
CHATTERINO_CLANG_TIDY_MODULE_CLANG_TIDY_HASH=0b591a512760d13eb3d089df21991830f1131cfbeac3f1c26d44c267573fb04b

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

curl -sSLo build-clang-tidy/clazy.zip \
    https://github.com/Nerixyz/clazy-ubuntu-builds/releases/download/ci/clazy-$CLAZY_VERSION-clang$CLANG_TIDY_VERSION.zip
echo "$CLAZY_CLANG_TIDY_HASH build-clang-tidy/clazy.zip" | sha256sum --check --status
unzip build-clang-tidy/clazy.zip -d /usr/lib

curl -sSLo build-clang-tidy/chatterino-clang-tidy-module.zip \
    https://github.com/Chatterino/clang-tidy-module/releases/download/ci/chatterino-clang-tidy-module-clang${CLANG_TIDY_VERSION}.zip
echo "$CHATTERINO_CLANG_TIDY_MODULE_CLANG_TIDY_HASH build-clang-tidy/chatterino-clang-tidy-module.zip" | sha256sum --check --status
unzip build-clang-tidy/chatterino-clang-tidy-module.zip -d /usr/lib

# Run MOC and UIC
# This will compile the dependencies
# Get the targets using `ninja -t targets | grep autogen`
cmake --build build-clang-tidy --parallel -t \
    Core_autogen \
    LibCommuni_autogen \
    Model_autogen \
    Util_autogen \
    chatterino-lib_autogen
