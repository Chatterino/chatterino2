# Linux

For all dependencies below we use Qt 6. Our minimum supported version is Qt 5.15.2, but you are on your own.

## Install dependencies

### Ubuntu

Building on Ubuntu requires Docker.

Use <https://github.com/Chatterino/docker/pkgs/container/chatterino2-build-ubuntu-20.04> as your base if you're on Ubuntu 20.04.

Use <https://github.com/Chatterino/docker/pkgs/container/chatterino2-build-ubuntu-22.04> if you're on Ubuntu 22.04.

The built binary should be exportable from the final image & able to run on your system assuming you perform a static build. See our [build.yml GitHub workflow file](.github/workflows/build.yml) for the CMake line used for Ubuntu builds.

### Debian 12 (bookworm) or later

```sh
sudo apt install qt6-base-dev qt6-5compat-dev qt6-svg-dev qt6-image-formats-plugins libboost1.81-dev libnotify-dev libssl-dev cmake g++ git
```

### Arch Linux

```sh
sudo pacman -S --needed qt6-base qt6-tools boost-libs openssl qt6-imageformats qt6-5compat qt6-svg boost libnotify rapidjson pkgconf openssl cmake
```

If you use Wayland, you will also need to ensure `qt6-wayland` is installed.

Alternatively you can use the [chatterino2-git](https://aur.archlinux.org/packages/chatterino2-git/) package to build and install Chatterino for you.

### Fedora 39 and above

_Most likely works the same for other Red Hat-like distros. Substitute `dnf` with `yum`._

```sh
sudo dnf install qt6-qtbase-devel qt6-qtimageformats qt6-qtsvg-devel qt6-qt5compat-devel g++ git openssl-devel boost-devel libnotify-devel cmake
```

### NixOS 18.09+

```sh
nix-shell -p openssl boost qt6.full pkg-config cmake libnotify
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
   cmake -DBUILD_WITH_QTKEYCHAIN=OFF ..
   ```
1. Build the project
   ```sh
   cmake --build .
   ```

### Through Qt Creator

1. Install C++ IDE Qt Creator by using `sudo apt install qtcreator` (Or whatever equivalent for your distro)
1. Open `CMakeLists.txt` with Qt Creator and select build
