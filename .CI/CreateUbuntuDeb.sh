#!/bin/sh

set -e

breakline() {
    printf "================================================================================\n\n"
}

# Configured in the CI step
install_prefix="appdir/usr"

# The directory we finally pack into our .deb package
packaging_dir="package"

# Get the Ubuntu Release (e.g. 20.04 or 22.04)
ubuntu_release="$(lsb_release -rs)"

# Refactor opportunity:
case "$ubuntu_release" in
    20.04)
        dependencies="libc6, libstdc++6, libqt5core5a, libqt5concurrent5, libqt5dbus5, libqt5gui5, libqt5network5, libqt5svg5, libqt5widgets5, libboost-filesystem1.71.0"
        ;;
    22.04)
        dependencies="libc6, libstdc++6, libqt5core5a, libqt5concurrent5, libqt5dbus5, libqt5gui5, libqt5network5, libqt5svg5, libqt5widgets5, libboost-filesystem1.74.0"
        ;;
    *)
        echo "Unsupported Ubuntu release $ubuntu_release"
        exit 1
        ;;
esac

echo "Building Ubuntu .deb file on '$ubuntu_release'"
echo "Dependencies: $dependencies"

if [ ! -f ./bin/chatterino ] || [ ! -x ./bin/chatterino ]; then
    echo "ERROR: No chatterino binary file found. This script must be run in the build folder, and chatterino must be built first."
    exit 1
fi

chatterino_version=$(git describe 2>/dev/null | cut -c 2-) || true
if [ -z "$chatterino_version" ]; then
    # Fall back to this in case the build happened outside of a git repo
    chatterino_version="0.0.0-dev"
fi

# Make sure no old remnants of a previous packaging remains
rm -vrf "$packaging_dir"

mkdir -p "$packaging_dir/DEBIAN"

echo "Making control file"
cat >> "$packaging_dir/DEBIAN/control" << EOF
Package: chatterino
Version: $chatterino_version
Architecture: amd64
Maintainer: Mm2PL <mm2pl@kotmisia.pl>
Depends: $dependencies
Section: net
Priority: optional
Homepage: https://github.com/Chatterino/chatterino2
Description: Ubuntu package built for $ubuntu_release
EOF
cat "$packaging_dir/DEBIAN/control"
breakline


echo "Running make install"
make install
find "$install_prefix"
breakline


echo "Merge install into packaging dir"
cp -rv "$install_prefix/" "$packaging_dir/"
find "$packaging_dir"
breakline


echo "Building package"
dpkg-deb --build "$packaging_dir" "Chatterino-x86_64.deb"
breakline


echo "Package info"
dpkg --info Chatterino-x86_64.deb
breakline


echo "Package contents"
dpkg --contents Chatterino-x86_64.deb # Shows folders and files inside .deb file
breakline
