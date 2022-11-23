#!/bin/sh
set -e

if [ ! -f ./bin/chatterino ] || [ ! -x ./bin/chatterino ]; then
    echo "ERROR: No chatterino binary file found. This script must be run in the build folder, and chatterino must be built first."
    exit 1
fi

chatterino_version=$(git describe 2>/dev/null | cut -c 2-) || true
if [ -z "$chatterino_version" ]; then
    chatterino_version="0.0.0-dev"
    echo "Falling back to setting the version to '$chatterino_version'"
else
    echo "Found Chatterino version $chatterino_version via git"
fi

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
Depends: qttools5-dev, qtmultimedia5-dev, qt5-image-formats-plugins, libqt5svg5-dev, libboost-dev, libssl-dev, libboost-system-dev, libboost-filesystem-dev, libqt5core5a
EOF
echo "Version: $chatterino_version" >> "$packaging_dir/DEBIAN/control"

echo "Running make install in package dir"
DESTDIR="$packaging_dir" make INSTALL_ROOT="$packaging_dir" -j"$(nproc)" install; find "$packaging_dir/"
echo ""

# move directory up
mv "$packaging_dir$(pwd)/appdir/usr" "$packaging_dir/"
rm -vrf "$packaging_dir/home" || true

echo "Building package..."
dpkg-deb --build "$packaging_dir" "Chatterino-x86_64.deb"
