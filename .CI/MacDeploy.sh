#!/usr/bin/env bash

# Bundle relevant qt & system dependencies into the ./chatterino.app folder

set -eo pipefail

if [ -d bin/chatterino.app ] && [ ! -d chatterino.app ]; then
    >&2 echo "Moving bin/chatterino.app down one directory"
    mv bin/chatterino.app chatterino.app
fi

if [ -n "$Qt5_DIR" ]; then
    echo "Using Qt DIR from Qt5_DIR: $Qt5_DIR"
    _QT_DIR="$Qt5_DIR"
    _img_version="5.15.2"
elif [ -n "$Qt6_DIR" ]; then
    echo "Using Qt DIR from Qt6_DIR: $Qt6_DIR"
    _QT_DIR="$Qt6_DIR"
    _img_version="6.5.0"
fi

if [ -n "$_QT_DIR" ]; then
    export PATH="${_QT_DIR}/bin:$PATH"
else
    echo "No Qt environment variable set, assuming system-installed Qt"
fi

echo "Running MACDEPLOYQT"

_macdeployqt_args=()

if [ -n "$MACOS_CODESIGN_CERTIFICATE" ]; then
    _macdeployqt_args+=("-codesign=$MACOS_CODESIGN_CERTIFICATE")
fi

macdeployqt chatterino.app "${_macdeployqt_args[@]}"

# Download kimageformats plugins

curl -SsfLo kimg.zip "https://github.com/jurplel/kimageformats-binaries/releases/download/cont/kimageformats-macos-latest-$_img_version.zip"
7z e -okimg kimg.zip
cp kimg/libKF5Archive.5.dylib chatterino.app/Contents/Frameworks/
cp kimg/kimg_avif.so chatterino.app/Contents/PlugIns/imageformats/

if [ -n "$MACOS_CODESIGN_CERTIFICATE" ]; then
    # Validate that chatterino.app was codesigned correctly
    codesign -v chatterino.app
fi
