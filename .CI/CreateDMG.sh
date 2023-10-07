#!/usr/bin/env bash

set -eo pipefail

if [ ! -d chatterino.app ]; then
    echo "ERROR: No 'chatterino.app' dir found in the build directory. Make sure you've run ./CI/MacDeploy.sh"
    exit 1
fi

if [ -z "$OUTPUT_DMG_PATH" ]; then
    echo "ERROR: Must specify the path for where to save the final .dmg. Make sure you've set the OUTPUT_DMG_PATH environment variable."
    exit 1
fi

if [ -z "$SKIP_VENV" ]; then
    echo "Creating python3 virtual environment"
    python3 -m venv venv
    echo "Entering python3 virtual environment"
    . venv/bin/activate
    echo "Installing dmgbuild"
    python3 -m pip install dmgbuild
fi

if [ -n "$MACOS_CODESIGN_CERTIFICATE" ]; then
    echo "Codesigning force deep inside the app"
    codesign -s "$MACOS_CODESIGN_CERTIFICATE" --deep --force chatterino.app
    echo "Done!"
fi

echo "Running dmgbuild.."
dmgbuild --settings ./../.CI/dmg-settings.py -D app=./chatterino.app Chatterino2 "$OUTPUT_DMG_PATH"
echo "Done!"

if [ -n "$MACOS_CODESIGN_CERTIFICATE" ]; then
    echo "Codesigning the dmg"
    codesign -s "$MACOS_CODESIGN_CERTIFICATE" --deep --force "$OUTPUT_DMG_PATH"
    echo "Done!"
fi
