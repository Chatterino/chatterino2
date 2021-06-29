#!/bin/sh
set -e

if [ ! -f ./bin/chatterino ] || [ ! -x ./bin/chatterino ]; then
    echo "ERROR: No chatterino binary file found. This script must be run in the build folder, and chatterino must be built first."
    exit 1
fi

# Chatterino's native version format is "Chatterino 2.3.3 (commit 6355742b)",
# this little magic contraption transforms that into "2.3.3+6355742b"
chatterino_version=$(bin/chatterino --version 2>&1 | tail -n 1 | sed 's/Chatterino //;s/ (commit /+/;s/)//')
echo "Found Chatterino version $chatterino_version"

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
EOF
echo "Version: $chatterino_version" >> "$packaging_dir/DEBIAN/control"

echo "Running make install in package dir"
DESTDIR="$packaging_dir" make INSTALL_ROOT="$packaging_dir" -j"$(nproc)" install; find "$packaging_dir/"
echo ""

echo "Building package..."
dpkg-deb --build "$packaging_dir" "chatterino-$chatterino_version.deb"
