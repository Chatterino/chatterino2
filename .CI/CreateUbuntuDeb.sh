#!/bin/sh

set -e

breakline() {
    printf "================================================================================\n\n"
}

# Configured in the CI step
install_prefix="appdir/usr"

# The directory we finally pack into our .deb package
packaging_dir="package"

# Get the Ubuntu Release (e.g. 20.04, 22.04 or 24.04)
ubuntu_release="$(lsb_release -rs)"

# The final path where we'll save the .deb package
deb_path="Chatterino-ubuntu-${ubuntu_release}-x86_64.deb"

# Refactor opportunity:
case "$ubuntu_release" in
    20.04)
        # Qt6 static-linked deb, see https://github.com/Chatterino/docker
        dependencies="libc6, libstdc++6, libblkid1, libbsd0, libexpat1, libffi7, libfontconfig1, libfreetype6, libglib2.0-0, libglvnd0, libglx0, libgraphite2-3, libharfbuzz0b, libicu66, libjpeg-turbo8, libmount1, libnotify4, libopengl0, libpcre2-16-0, libpcre3, libpng16-16, libselinux1, libssl1.1, libuuid1, libx11-xcb1, libxau6, libxcb1, libxcb-cursor0, libxcb-glx0, libxcb-icccm4, libxcb-image0, libxcb-keysyms1, libxcb-randr0, libxcb-render0, libxcb-render-util0, libxcb-shape0, libxcb-shm0, libxcb-sync1, libxcb-util1, libxcb-xfixes0, libxcb-xkb1, libxdmcp6, libxkbcommon0, libxkbcommon-x11-0, zlib1g"
        ;;
    22.04)
        # Qt6 static-linked deb, see https://github.com/Chatterino/docker
        dependencies="libc6, libstdc++6, libglx0, libopengl0, libpng16-16, libharfbuzz0b, libfreetype6, libfontconfig1, libjpeg-turbo8, libxcb-glx0, libegl1, libx11-6, libxkbcommon0, libx11-xcb1, libxkbcommon-x11-0, libxcb-cursor0, libxcb-icccm4, libxcb-image0, libxcb-keysyms1, libxcb-randr0, libxcb-render-util0, libxcb-shm0, libxcb-sync1, libxcb-xfixes0, libxcb-render0, libxcb-shape0, libxcb-xkb1, libxcb1, libbrotli1, libglib2.0-0, zlib1g, libicu70, libpcre2-16-0, libssl3, libgraphite2-3, libexpat1, libuuid1, libxcb-util1, libxau6, libxdmcp6, libffi8, libmount1, libnotify4, libselinux1, libpcre3, libbsd0, libblkid1, libpcre2-8-0, libmd0"
        ;;
    24.04)
        # Qt6 static-linked deb, see https://github.com/Chatterino/docker
        dependencies="libc6, libstdc++6, libglx0, libopengl0, libpng16-16, libharfbuzz0b, libfreetype6, libfontconfig1, libjpeg-turbo8, libxcb-glx0, libegl1, libx11-6, libxkbcommon0, libx11-xcb1, libxkbcommon-x11-0, libxcb-cursor0, libxcb-icccm4, libxcb-image0, libxcb-keysyms1, libxcb-randr0, libxcb-render-util0, libxcb-shm0, libxcb-sync1, libxcb-xfixes0, libxcb-render0, libxcb-shape0, libxcb-xkb1, libxcb1, libbrotli1, libglib2.0-0, zlib1g, libicu74, libpcre2-16-0, libssl3, libgraphite2-3, libexpat1, libuuid1, libxcb-util1, libxau6, libxdmcp6, libffi8, libmount1, libnotify4, libselinux1, libpcre3, libbsd0, libblkid1, libpcre2-8-0, libmd0"
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

chatterino_version=$(git describe 2>/dev/null) || true
if [ "$(echo "$chatterino_version" | cut -c1-1)" = 'v' ]; then
    chatterino_version="$(echo "$chatterino_version" | cut -c2-)"
else
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
dpkg-deb --build "$packaging_dir" "$deb_path"
breakline


echo "Package info"
dpkg --info "$deb_path"
breakline


echo "Package contents"
dpkg --contents "$deb_path"
breakline
