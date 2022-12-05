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
Depends: rapidjson-dev, libfuse2, libssl-dev, libboost-dev, libxcb-randr0-dev, libbost-system-dev, libboost-filesystem-dev, libxkbcommon-x11-0, libgstreamer-plugins-base1.0-0, libgl1-mesa-dev, libxcb-icccm4, libxcb-image0, libxcb-keysyms1, libxcb-render-util0, libxcb-xinerama0
EOF
echo "Version: $chatterino_version" >> "$packaging_dir/DEBIAN/control"

echo "Running make install in package dir"
DESTDIR="$packaging_dir" make INSTALL_ROOT="$packaging_dir" -j"$(nproc)" install; find "$packaging_dir/"
echo ""

echo "$packaging_dir$(pwd)/appdir/usr"
echo "$packaging_dir/"
# move directory up
mv "$packaging_dir$(pwd)/appdir/usr" "$packaging_dir/"
rm -vrf "$packaging_dir/home" || true

echo "Building package..."
dpkg-deb --build "$packaging_dir" "Chatterino-x86_64.deb"
dpkg -I Chatterino-x86_64.deb
dpkg -c Chatterino-x86_64.deb
