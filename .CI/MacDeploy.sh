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

# Copy dynamic library dependencies into the Frameworks directory
cp /opt/universal-lib/libcrypto.dylib chatterino.app/Contents/Frameworks/libcrypto.3.dylib
cp /opt/universal-lib/libssl.dylib chatterino.app/Contents/Frameworks/libssl.3.dylib

# Fix the library IDs to match their new location
install_name_tool -id @executable_path/../Frameworks/libssl.3.dylib chatterino.app/Contents/Frameworks/libssl.3.dylib
install_name_tool -id @executable_path/../Frameworks/libcrypto.3.dylib chatterino.app/Contents/Frameworks/libcrypto.3.dylib

otool -L chatterino.app/Contents/Frameworks/libssl.3.dylib
# Fix the search path for libcrypto in libssl
otool -L chatterino.app/Contents/Frameworks/libssl.3.dylib \
    | grep libcrypto.3.dylib \
    | cut -d" " -f1 \
    | cut -f2 \
    | while read input_crypto_dylib; do \
        install_name_tool -change \
            "$input_crypto_dylib" \
            @executable_path/../Frameworks/libcrypto.3.dylib \
            chatterino.app/Contents/Frameworks/libssl.3.dylib; \
    done
# Fix the search path for lib{crypto,ssl} in chatterino
otool -L chatterino.app/Contents/MacOS/chatterino \
    | grep /opt \
    | cut -d" " -f1 \
    | cut -f2 \
    | while read og_entry; do \
        echo $og_entry; \
        install_name_tool -change \
            "$og_entry" \
            @executable_path/../Frameworks/$(echo $og_entry | sed -E 's/.*(libssl|libcrypto)/\1/') \
            chatterino.app/Contents/MacOS/chatterino; \
        done

