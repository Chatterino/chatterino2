#!/bin/sh

set -e

# Print all commands as they are run
set -x

if [ ! -f ./bin/chatterino ] || [ ! -x ./bin/chatterino ]; then
    echo "ERROR: No chatterino binary file found. This script must be run in the build folder, and chatterino must be built first."
    exit 1
fi

if [ -n "$Qt5_DIR" ]; then
    echo "Using Qt DIR from Qt5_DIR: $Qt5_DIR"
    _QT_DIR="$Qt5_DIR"
elif [ -n "$Qt6_DIR" ]; then
    echo "Using Qt DIR from Qt6_DIR: $Qt6_DIR"
    _QT_DIR="$Qt6_DIR"
fi

if [ -n "$_QT_DIR" ]; then
    export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:${_QT_DIR}/lib"
    export PATH="${_QT_DIR}/bin:$PATH"
else
    echo "No Qt environment variable set, assuming system-installed Qt"
fi

script_path=$(readlink -f "$0")
script_dir=$(dirname "$script_path")
chatterino_dir=$(dirname "$script_dir")


echo "Running LDD on chatterino binary:"
ldd ./bin/chatterino
echo ""

echo "Running make install in the appdir"
make INSTALL_ROOT=appdir -j"$(nproc)" install ; find appdir/
echo ""

cp "$chatterino_dir"/resources/icon.png ./appdir/chatterino.png

linuxdeployqt_path="linuxdeployqt-x86_64.AppImage"
linuxdeployqt_url="https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"

if [ ! -f "$linuxdeployqt_path" ]; then
    echo "Downloading LinuxDeployQT from $linuxdeployqt_url to $linuxdeployqt_path"
    curl --location --fail --silent "$linuxdeployqt_url" -o "$linuxdeployqt_path"
    chmod a+x "$linuxdeployqt_path"
fi

appimagetool_path="appimagetool-x86_64.AppImage"
appimagetool_url="https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"

if [ ! -f "$appimagetool_path" ]; then
    echo "Downloading AppImageTool from $appimagetool_url to $appimagetool_path"
    curl --location --fail --silent "$appimagetool_url" -o "$appimagetool_path"
    chmod a+x "$appimagetool_path"
fi

# For some reason, the copyright file for libc was not found. We need to manually copy it from the host system
mkdir -p appdir/usr/share/doc/libc6/
cp /usr/share/doc/libc6/copyright appdir/usr/share/doc/libc6/

echo "Run LinuxDeployQT"
./"$linuxdeployqt_path" \
    --appimage-extract-and-run \
    appdir/usr/share/applications/com.chatterino.chatterino.desktop \
    -no-translations \
    -bundle-non-qt-libs \
    -unsupported-allow-new-glibc

rm -rf appdir/home
rm -f appdir/AppRun

echo "Run AppImageTool"

# shellcheck disable=SC2016
echo '#!/bin/sh
here="$(dirname "$(readlink -f "${0}")")"
export QT_QPA_PLATFORM_PLUGIN_PATH="$here/usr/plugins"
cd "$here/usr"
exec "$here/usr/bin/chatterino" "$@"' > appdir/AppRun
chmod a+x appdir/AppRun

./"$appimagetool_path" \
    --appimage-extract-and-run \
    appdir

# TODO: Create appimage in a unique directory instead maybe idk?
rm -rf appdir
