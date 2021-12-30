#!/bin/sh
set -e

if [ ! -f ./bin/chatterino ] || [ ! -x ./bin/chatterino ]; then
    echo "ERROR: No chatterino binary file found. This script must be run in the build folder, and chatterino must be built first."
    exit 1
fi

if grep qmake Makefile; then
    echo "ERROR: Ubuntu packaging doesn't support qmake."
    exit 1
fi

chatterino_version=$(git describe | cut -c 2-)
echo "Found Chatterino version $chatterino_version via git ($(pwd))"

rm -vrf "./package" || true  # delete any old packaging dir

# create ./package/ from scratch
mkdir package/DEBIAN -p
packaging_dir="$(realpath ./package)"

echo "Making control file"
cat >> "$packaging_dir/DEBIAN/control" << EOF
Package: chatterino
Section: net
Priority: optional
Architecture: amd64
Maintainer: Mm2PL <mm2pl@kotmisia.pl>
Description: Testing out chatterino as a Ubuntu package
Depends: libc6, libqt5concurrent5, libqt5core5a, libqt5dbus5, libqt5gui5, libqt5multimedia5, libqt5network5, libqt5svg5, libqt5widgets5, libssl1.1, libstdc++6
EOF
echo "Version: $chatterino_version" >> "$packaging_dir/DEBIAN/control"

rm -rf appdir  # clean out static libraries
echo "Installing into appdir"  # if using cmake, can't change install dir
make INSTALL_ROOT=appdir -j"$(nproc)" install
echo "Reusing appdir data..."
cp -r appdir/* "$packaging_dir"/

find "$packaging_dir/"
echo ""

echo "Building package..."
cd "$packaging_dir"
dpkg-deb --build "$packaging_dir" "../Chatterino.deb"
