# Linux

Note on Qt version compatibility: If you are installing Qt from a package manager, please ensure the version you are installing is at least **Qt 5.12 or newer**.

## Ubuntu 18.04

_Most likely works the same for other Debian-like distros_

1. Install all of the dependencies using `sudo apt install qttools5-dev qtmultimedia5-dev libqt5svg5-dev libboost-dev libssl-dev libboost-system-dev libboost-filesystem-dev cmake g++`

### Compiling through Qt Creator

1. Install C++ IDE Qt Creator by using `sudo apt install qtcreator`
1. Open `chatterino.pro` with Qt Creator and select build

### Manually

1. Go into the project directory
1. Create a build folder and go into it (`mkdir build && cd build`)
1. Use one of the options below to compile it

### Using CMake

`cmake .. && make`

### Using QMake

`qmake .. && make`

## Arch Linux

### Through AUR

- [chatterino2-git](https://aur.archlinux.org/packages/chatterino2-git/)

### Manually

1. Install all of the dependencies using `sudo pacman -S qt5-base qt5-multimedia qt5-svg qt5-tools gst-plugins-ugly gst-plugins-good boost rapidjson pkgconf openssl cmake`
1. Go into the project directory
1. Create a build folder and go into it (`mkdir build && cd build`)
1. Use one of the options below to compile it

### Using CMake

`cmake .. && make`

### Using QMake

`qmake .. && make`

## Fedora 28 and above

_Most likely works the same for other Red Hat-like distros. Substitute `dnf` with `yum`._

1. Install all of the dependencies using `sudo dnf install qt5-qtbase-devel qt5-qtmultimedia-devel qt5-qtsvg-devel libsecret-devel openssl-devel boost-devel cmake`
1. Go into the project directory
1. Create a build folder and go into it (`mkdir build && cd build`)
1. Use one of the options below to compile it

### Using CMake

`cmake .. && make -j$(nproc)`

### Using QMake

`qmake-qt5 .. && make -j$(nproc)`

## NixOS 18.09+

1. Enter the development environment with all of the dependencies: `nix-shell -p openssl boost qt5.full pkg-config cmake`
1. Go into the project directory
1. Create a build folder and go into it (`mkdir build && cd build`)
1. Use one of the options below to compile it

### Using CMake

`cmake .. && make`

### Using QMake

`qmake .. && make`
