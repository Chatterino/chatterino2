# Linux

## Ubuntu 18.04
*most likely works the same for other Debian-like distros*
1. Install dependencies (and the C++ IDE Qt Creator) `sudo apt install qtcreator qtmultimedia5-dev libqt5svg5-dev libboost-dev libssl-dev libboost-system-dev`
2. Open `chatterino.pro` with QT Creator and build

## Arch Linux
install [chatterino2-git](https://aur.archlinux.org/packages/chatterino2-git/) from the aur or build manually as follows:
1. `sudo pacman -S qt5-base qt5-multimedia qt5-svg gst-plugins-ugly gst-plugins-good boost rapidjson`
2. go into project directory
3. create build folder `mkdir build && cd build`
4. `qmake .. && make`
