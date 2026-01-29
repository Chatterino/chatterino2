# Linux

For all dependencies below we use Qt 6. Our minimum supported version is Qt 5.15.2, but you are on your own.

## Install dependencies

### Ubuntu

Building on Ubuntu requires Docker.

Use <https://github.com/Chatterino/docker/pkgs/container/chatterino2-build-ubuntu-22.04> if you're on Ubuntu 22.04.

Use <https://github.com/Chatterino/docker/pkgs/container/chatterino2-build-ubuntu-24.04> if you're on Ubuntu 24.04.

The built binary should be exportable from the final image & able to run on your system assuming you perform a static build. See our [build.yml GitHub workflow file](.github/workflows/build.yml) for the CMake line used for Ubuntu builds.

### Debian 13 (trixie) or later

```sh
sudo apt install qt6-base-dev qt6-svg-dev qt6-image-formats-plugins libboost-dev libnotify-dev libssl-dev libsecret-1-dev pkg-config cmake g++ git hunspell
```

### Arch Linux

```sh
sudo pacman -S --needed qt6-base qt6-tools boost-libs openssl qt6-imageformats qt6-svg boost libnotify rapidjson pkgconf cmake hunspell
```

If you use Wayland, you will also need to ensure `qt6-wayland` is installed.

Alternatively you can use the [chatterino2-git](https://aur.archlinux.org/packages/chatterino2-git/) package to build and install Chatterino for you.

### openSUSE

```sh
sudo zypper install cmake pkgconf boost-devel libboost_json1_89_0-devel desktop-file-utils libappstream-glib8 hunspell ninja doxygen qt6-tools-devel
```

### Gentoo Linux

```sh
doas emerge dev-libs/openssl dev-qt/qt5compat dev-qt/qtbase dev-qt/qtsvg dev-qt/qtimageformats x11-libs/libnotify dev-libs/qtkeychain dev-libs/boost dev-build/cmake app-text/hunspell
```

### Fedora 39 and above

_Most likely works the same for other Red Hat-like distros. Substitute `dnf` with `yum`._

```sh
sudo dnf install qt6-qtbase-devel qt6-qtimageformats qt6-qtsvg-devel g++ git openssl-devel boost-devel libnotify-devel cmake hunspell
```

### NixOS 18.09+

```sh
nix-shell -p openssl boost qt6.full pkg-config cmake libnotify hunspell
```

## Compile

## Manually

1. In the project directory, create a build directory and enter it
   ```sh
   mkdir build
   cd build
   ```
1. Generate build files. To enable Lua plugins in your build add `-DCHATTERINO_PLUGINS=ON` to this command.
   ```sh
   cmake -DBUILD_WITH_QTKEYCHAIN=OFF -DCHATTERINO_SPELLCHECK=On ..
   ```
1. Build the project
   ```sh
   cmake --build .
   ```

### Through Qt Creator

1. Install C++ IDE Qt Creator by using `sudo apt install qtcreator` (Or whatever equivalent for your distro)
1. Open `CMakeLists.txt` with Qt Creator and select build
