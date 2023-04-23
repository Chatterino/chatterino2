#!/usr/bin/env bash

set -eo pipefail

if [ -d bin/chatterino.app ] && [ ! -d chatterino.app ]; then
    >&2 echo "Moving bin/chatterino.app down one directory"
    mv bin/chatterino.app chatterino.app
fi

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

macdeployqt chatterino.app "${_macdeployqt_args[@]}"

if [ -n "$MACOS_CODESIGN_CERTIFICATE" ]; then
    # Validate that chatterino.app was codesigned correctly
    codesign -v chatterino.app
fi

if [ -z "$SKIP_VENV" ]; then
    echo "Creating python3 virtual environment"
    python3 -m venv venv
    echo "Entering python3 virtual environment"
    . venv/bin/activate
    echo "Installing dmgbuild"
    python3 -m pip install dmgbuild
fi

_dmg_path="chatterino-macos-Qt-$1.dmg"

echo "Running dmgbuild.."
dmgbuild --settings ./../.CI/dmg-settings.py -D app=./chatterino.app Chatterino2 "$_dmg_path"
echo "Done!"

if [ -n "$MACOS_CODESIGN_CERTIFICATE" ]; then
    echo "Codesigning the dmg"
    codesign -s "$MACOS_CODESIGN_CERTIFICATE" --deep "$_dmg_path"
    echo "Done!"
fi
