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
1. Install Visual Studio 2017 and select "Desktop development with C++" and "Universal Windows Platform development.

##### Boost
1. Visual Studio 2017 64-bit: https://dl.bintray.com/boostorg/release/1.66.0/binaries/boost_1_66_0-msvc-14.1-64.exe
2. When prompted, install boost to C:\local\boost
3. When the installation is finished, go to C:\local\boost and rename the "lib64-msvc-14.1" folder to "lib"

##### OpenSSL
###### For our websocket library, we need OpenSSL 1.1
1. Download OpenSSL development library: https://slproweb.com/download/Win64OpenSSL-1_1_0h.exe
2. When prompted, install openssl to C:\local\openssl
3. When prompted, copy the OpenSSL DLLs to "The OpenSSL binaries (/bin) directory"
###### For Qt SSL, we need OpenSSL 1.0
1. Download OpenSSL light: https://slproweb.com/download/Win64OpenSSL_Light-1_0_2o.exe
2. When prompted, install it anywhere
3. When prompted, copy the OpenSSL DLLS to "The OpenSSL binaries (/bin) directory"
4. Copy the OpenSSL 1.0 files from its /bin folder to C:/local/bin (You will need to create the folder)
5. Then copy the OpenSSL 1.1 files from its /bin folder to C:/local/bin (Overwrite any duplicate files)
6. Add C:/local/bin to your path folder (Follow guide here if you don't know how to do it: https://www.computerhope.com/issues/ch000549.htm#windows8 )

##### Qt
1. Download Qt: https://www.qt.io/download
2. Select "Open source" at the bottom of this page
3. Then select "Download"
###### When prompted which components to install:
1. Under the latest Qt version:
  - Select MSVC 2017 64-bit (or MSVC 2015 64-bit if you still use Visual Studio 2015)
  - Optionally, enable Qt WebEngine
2. Under Tools:
  - Select Qt Creator, and Qt Creator CDB Debugger Support


### Windows (Using MSYS2)
Note: This guide is currently out of date and will not work as is.
Note: This build will have some features missing from the build.

Building using MSYS2 can be quite easier process. Check out MSYS2 at [msys2.org](http://www.msys2.org/).

Be sure to add `-j <number of threads>` as a make argument so it will use all your cpu cores to build. [example setup](https://i.imgur.com/qlESlS1.png)

You can also add `-o2` to optimize the final binary size but increase compilation time, and add `-pipe` to use more ram in compilation but increase compilation speed
1. open appropriate MSYS2 terminal and do `pacman -S mingw-w64-<arch>-boost mingw-w64-<arch>-qt5 mingw-w64-<arch>-rapidjson` where `<arch>` is `x86_64` or `i686`
2. go into the project directory
3. create build folder `mkdir build && cd build`
4. `qmake .. && mingw32-make`

### 

### Linux
#### Ubuntu 16.04.2 LTS
*most likely works the same for other Debian-like distros*
1. install QT Creator `sudo apt-get install qtcreator qtmultimedia5-dev`
2. install boost-dev  `sudo apt-get install libboost-dev`
3. open `chatterino.pro` with QT Creator and build

#### Ubuntu 18.04
*most likely works the same for other Debian-like distros*
1. Install dependencies (and the C++ IDE Qt Creator) `sudo apt install qtcreator qtmultimedia5-dev libqt5svg5-dev libboost-dev`
2. Install rapidjson to `/usr/local/` like this: From the Chatterino2 root folder: `sudo cp -r lib/rapidjson/include/rapidjson /usr/local/include`. If you want to install it to another place, you have to make sure it's in the chatterino.pro include path
3. open `chatterino.pro` with QT Creator and build

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

Test 1
