# Linux

Note on Qt version compatibility: If you are installing Qt from a package manager, please ensure the version you are installing is at least **Qt 5.10 or newer**.

## Ubuntu 18.04
*most likely works the same for other Debian-like distros*
1. Install dependencies (and the C++ IDE Qt Creator) `sudo apt install qtcreator qtmultimedia5-dev libqt5svg5-dev libboost-dev libssl-dev libboost-system-dev libboost-filesystem-dev`
1. Open `chatterino.pro` with QT Creator and build

## Arch Linux
install [chatterino2-git](https://aur.archlinux.org/packages/chatterino2-git/) from the aur or build manually as follows:
1. `sudo pacman -S qt5-base qt5-multimedia qt5-svg gst-plugins-ugly gst-plugins-good boost rapidjson`
1. go into project directory
1. create build folder `mkdir build && cd build`
1. `qmake .. && make`

## Fedora 28 and above
*most likely works the same for other Red Hat-like distros. Substitue `dnf` with `yum`.*
### Development dependencies
1. `sudo dnf install qt5-qtbase-devel qt5-qtmultimedia-devel qt5-qtsvg-devel libsecret-devel openssl-devel boost-devel`
1. go into project directory
1. create build folder `mkdir build && cd build`
1. `qmake-qt5 .. && make -j$(nproc)`
### Optional dependencies
*`gstreamer-plugins-good` package is retired in Fedora 31, see: [rhbz#1735324](https://bugzilla.redhat.com/show_bug.cgi?id=1735324)*
1. `sudo dnf install gstreamer-plugins-good` *(optional: for audio output)*

## NixOS 18.09+
1. enter the development environment with all of the dependencies: `nix-shell -p openssl boost qt5.full`
1. go into project directory
1. create build folder `mkdir build && cd build`
1. `qmake .. && make`
