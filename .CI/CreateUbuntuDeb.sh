#!/bin/sh
set -e

#ubuntu_release=$(cat /etc/lsb-release | grep '^DISTRIB_RELEASE=*' | sed -e 's/DISTRIB_RELEASE=//g')
ubuntu_release=$(cat /etc/lsb-release | sed -n 's/^DISTRIB_RELEASE=//p')
echo $ubuntu_release # Test

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
# Backup-Depends: qtmultimedia5-dev, cmake, g++, rapidjson-dev, libfuse2, libssl-dev, libboost-dev, libxcb-randr0-dev, libboost-system-dev, libboost-filesystem-dev, libxkbcommon-x11-0, libgstreamer-plugins-base1.0-0, libgl1-mesa-dev, libxcb-icccm4, libxcb-image0, libxcb-keysyms1, libxcb-render-util0, libxcb-xinerama0, qttools5-dev, libqt5multimedia5, qt5-image-formats-plugins, libqt5svg5-dev, libqt5core5a, libcrypto++6
cat >> "$packaging_dir/DEBIAN/control" << EOF
Package: chatterino
Section: net
Priority: optional
Architecture: amd64
Maintainer: Mm2PL <mm2pl@kotmisia.pl>
Description: Testing out chatterino as a Ubuntu package
Depends: libboost-filesystem-dev, libxcb-xinerama0, libdbus-1-3
EOF
echo "Version: $chatterino_version" >> "$packaging_dir/DEBIAN/control"
cat >> "$packaging_dir/DEBIAN/postinst" << EOF
export LD_LIBRARY_PATH=/lib/qt/Qt/5.15.2/gcc_64/lib:"$LD_LIBRARY_PATH"
EOF
chmod 555 "$packaging_dir/DEBIAN/postinst"

cp -R ./appdir/usr/bin $packaging_dir/usr/bin
cp -R ./appdir/usr/share $packaging_dir/usr/share
# echo "Running make install in package dir"
# DESTDIR="$packaging_dir" make INSTALL_ROOT="$packaging_dir" -j"$(nproc)" install; find "$packaging_dir/"
# DESTDIR="$packaging_dir" make -j"$(nproc)" install; find "$packaging_dir/"
# make -j"$(nproc)" install; find "$packaging_dir/"
echo ""

echo "$packaging_dir$(pwd)/appdir/usr"
echo "$packaging_dir/"
# move directory up
# mv "$packaging_dir$(pwd)/appdir/usr" "$packaging_dir/" # remove INSTALL_ROOT

# mkdir -p "$packaging_dir/lib/qt"
# cp -R "../qt" "$packaging_dir/lib"
mkdir -p "$packaging_dir/lib/Qt/5.15.2/gcc_64/lib"
for i in "5.12.12" "5.15.2"; do
    for j in "libQt5Core" "libQt5Concurrent" "libQt5Gui" "libQt5Network" "libQt5Svg" "libQt5Widgets"; do
        cp ../qt/Qt/$i/gcc_64/lib/$j* $packaging_dir/lib/Qt/$i/gcc_64/lib/ 2>/dev/null || true
    done
done
# rm -vrf "$packaging_dir/home" || true

echo "Building package..."
dpkg-deb --build "$packaging_dir" "Chatterino-x86_64.deb"
dpkg -I Chatterino-x86_64.deb
dpkg -c Chatterino-x86_64.deb
