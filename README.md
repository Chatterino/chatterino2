![alt text](https://fourtf.com/img/chatterino-icon-64.png)
Chatterino 2
============

Chatterino 2 is the second installment of the Twitch chat client series "Chatterino". For now you can check out Chatterino 1 at [https://chatterino.com](https://chatterino.com).

## Code style
The code is normally formated using clang format in Qt Creator. [.clang-format](https://github.com/fourtf/chatterino2/blob/master/.clang-format) contains the style file for clang format.

To setup automatic code formating with QT Creator, see [this guide](https://gist.github.com/pajlada/0296454198eb8f8789fd6fe7ea660c5b).

## Building
Before building run `git submodule update --init --recursive` to get required submodules.

### Windows
#### Using Qt Creator
##### Visual Studio 2017
Install Visual Studio 2017 and select "Desktop development with C++" and "Universal Windows Platform development.
download the [boost library](https://sourceforge.net/projects/boost/files/boost/1.63.0/boost_1_63_0.zip/download) and extract it to `C:\local\boost`
##### open-ssl
1. download binaries for OpenSSL >= 1.0.2 or compile it from source. [example download](https://indy.fulgan.com/SSL/)
2. Place libeay32.dll and ssleay32.dll from OpenSSL in a directory in PATH.

#### Using MSYS2
Building using MSYS2 can be quite easier process. Check out MSYS2 at [msys2.org](http://www.msys2.org/).
Be sure to add "-j <number of cores\*2>" as a make argument so it will use all your cpu cores to build. [example setup](https://i.imgur.com/qlESlS1.png)
1. open appropriate MSYS2 terminal and do `pacman -S mingw-w64-<arch>-boost mingw-w64-<arch>-qt5 mingw-w64-<arch>-rapidjson` where `<arch>` is x86_64 or i686
2. go into the project directory
3. create build folder `mkdir build && cd build`
4. `qmake .. && mingw32-make`

### 

### Linux
#### Ubuntu 16.04.2 LTS
*most likely works the same for other Debian-like distros*
1. install QT Creator `sudo apt install qtcreator qtmultimedia5-dev libqt5svg5-dev`
2. install boost-dev  `sudo apt install libboost-dev`
3. copy `include/rapidjson` from [rapidjson](https://github.com/miloyip/rapidjson/releases/latest) into the chatterino directory so that the file `<chatterino2 directory>/rapidjson/document.h` exists
4. open `chatterino.pro` with QT Creator and build

#### Arch Linux
install [chatterino2-git](https://aur.archlinux.org/packages/chatterino2-git/) from the aur or build manually as follows:
1. `sudo pacman -S qt5-base qt5-multimedia gst-plugins-ugly gst-plugins-good boost rapidjson`
2. go into project directory
3. create build folder `mkdir build && cd build`
4. `qmake .. && make`

### Mac OSX
1. install Xcode and Xcode Command Line Utilites
2. install Qt Creator
3. install brew https://brew.sh/
4. `brew install boost openssl rapidjson`
5. build the project using Qt Creator
