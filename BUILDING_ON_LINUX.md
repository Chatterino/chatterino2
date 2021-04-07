# Linux

Note on Qt version compatibility: If you are installing Qt from a package manager, please ensure the version you are installing is at least **Qt 5.12 or newer**.

## Ubuntu 18.04

_most likely works the same for other Debian-like distros_

1. Install dependencies `sudo apt install qttools5-dev qtmultimedia5-dev libqt5svg5-dev libboost-dev libssl-dev libboost-system-dev libboost-filesystem-dev cmake g++`

### Through Qt Creator

1. Install C++ IDE Qt Creator `sudo apt install qtcreator`
1. Open `chatterino.pro` with Qt Creator and select build

### Manually

1. go into project directory
1. create build folder `mkdir build && cd build`

#### Using QMake

1. `qmake .. && make`

#### Using CMake

1. `cmake .. && make`

## Arch Linux

### Through AUR

- [chatterino2-git](https://aur.archlinux.org/packages/chatterino2-git/)

### Manually

1. `sudo pacman -S qt5-base qt5-multimedia qt5-svg qt5-tools gst-plugins-ugly gst-plugins-good boost rapidjson pkgconf openssl cmake`
1. go into project directory
1. create build folder `mkdir build && cd build`

#### Using QMake

1. `qmake .. && make`

#### Using CMake

1. `cmake .. && make`

## Fedora 28 and above

_most likely works the same for other Red Hat-like distros. Substitue `dnf` with `yum`._

1. `sudo dnf install qt5-qtbase-devel qt5-qtmultimedia-devel qt5-qtsvg-devel libsecret-devel openssl-devel boost-devel cmake`
1. go into project directory
1. create build folder `mkdir build && cd build`

### Using QMake

1. `qmake-qt5 .. && make -j$(nproc)`

### Using CMake

1. `cmake .. && make -j$(nproc)`

### Optional dependencies

_`gstreamer-plugins-good` package is retired in Fedora 31, see: [rhbz#1735324](https://bugzilla.redhat.com/show_bug.cgi?id=1735324)_

1. `sudo dnf install gstreamer-plugins-good` _(optional: for audio output)_

## NixOS 18.09+

1. enter the development environment with all of the dependencies: `nix-shell -p openssl boost qt5.full pkg-config cmake`
1. go into project directory
1. create build folder `mkdir build && cd build`

### Using QMake

1. `qmake .. && make`

### Using CMake

1. `cmake .. && make`
