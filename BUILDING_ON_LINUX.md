# Linux

Note on Qt version compatibility: If you are installing Qt from a package manager, please ensure the version you are installing is at least **Qt 5.15 or newer**.

## Install dependencies

### Ubuntu 20.04

_Most likely works the same for other Debian-like distros_

Install all of the dependencies using `sudo apt install qttools5-dev qtmultimedia5-dev qt5-image-formats-plugins libqt5svg5-dev libboost-dev libssl-dev libboost-system-dev libboost-filesystem-dev cmake g++`

### Arch Linux

Install all of the dependencies using `sudo pacman -S --needed qt5-base qt5-multimedia qt5-svg qt5-tools gst-plugins-ugly gst-plugins-good boost rapidjson pkgconf openssl cmake`

Alternatively you can use the [chatterino2-git](https://aur.archlinux.org/packages/chatterino2-git/) package to build and install Chatterino for you.

### Fedora 28 and above

_Most likely works the same for other Red Hat-like distros. Substitute `dnf` with `yum`._

Install all of the dependencies using `sudo dnf install qt5-qtbase-devel qt5-qtmultimedia-devel qt5-qtsvg-devel qt5-linguist libsecret-devel openssl-devel boost-devel cmake`

### NixOS 18.09+

Enter the development environment with all of the dependencies: `nix-shell -p openssl boost qt5.full pkg-config cmake`

## Compile

### Through Qt Creator

1. Install C++ IDE Qt Creator by using `sudo apt install qtcreator`
1. Open `CMakeLists.txt` with Qt Creator and select build

## Manually

1. In the project directory, create a build directory and enter it
   ```sh
   mkdir build
   cd build
   ```
1. Generate build files
   ```sh
   cmake ..
   ```
1. Build the project
   ```sh
   make
   ```
