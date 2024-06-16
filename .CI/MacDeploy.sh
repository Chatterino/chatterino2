#!/usr/bin/env bash

# Bundle relevant qt & system dependencies into the ./chatterino.app folder

set -eo pipefail

if [ ! -d bin/chatterino.app ]; then
    >&2 echo "No chatterino.app found under ./bin - rm -rf bin & build clean"
    exit
fi

if [ -d chatterino.app ]; then
    >&2 echo "chatterino.app found - deleting it"
    rm -rf chatterino.app
fi

if [ -d /opt/homebrew/chatterino.app ]; then
    >&2 echo "chatterino.app found under /opt/homebrew - deleting it"
    rm -rf /opt/homebrew/chatterino.app
fi

>&2 echo "Moving bin/chatterino.app to /opt/homebrew"
mv bin/chatterino.app /opt/homebrew/

if [ -n "$Qt5_DIR" ]; then
    echo "Using Qt DIR from Qt5_DIR: $Qt5_DIR"
    _QT_DIR="$Qt5_DIR"
elif [ -n "$Qt6_DIR" ]; then
    echo "Using Qt DIR from Qt6_DIR: $Qt6_DIR"
    _QT_DIR="$Qt6_DIR"
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

macdeployqt /opt/homebrew/chatterino.app "${_macdeployqt_args[@]}"

if [ -n "$MACOS_CODESIGN_CERTIFICATE" ]; then
    echo "Running codesign to validate"
    # Validate that chatterino.app was codesigned correctly
    codesign -v /opt/homebrew/chatterino.app
fi

>&2 echo "Moving /opt/homebrew/chatterino.app back"
mv /opt/homebrew/chatterino.app ./
