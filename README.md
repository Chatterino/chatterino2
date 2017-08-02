# chatterino 2

Chatterino 2 is the second installment of the twitch chat client series "Chatterino". For now you can check out chatterino 1 at [chatterino.com](http://chatterino.com).

## code style
The code is normally formated using clang format in qt creator. [.clang-format](https://github.com/fourtf/chatterino2/blob/master/.clang-format) contains the style file for clang format.

## requirements

### submodules
you need to run `git submodule update --init --recursive` to init the submodules

### windows
#### boost
download the [boost library](https://sourceforge.net/projects/boost/files/boost/1.63.0/boost_1_63_0.zip/download) and extract it to `C:\local\boost`
#### open-ssl
1. download binaries for OpenSSL >= 1.0.2 or compile it from source. [example download](https://indy.fulgan.com/SSL/)
2. Place libeay32.dll and ssleay32.dll from OpenSSL in a directory in PATH.
#### rapidjson
1. download rapidjson zip from https://github.com/miloyip/rapidjson/releases/latest
2. extract to `C:/local/rapidjson` so that `C:/local/rapidjson/include/rapidjson/` is a proper path

### linux
#### Ubuntu 16.04.2 LTS
*most likely works the same for other Debian-like distros*
1. Install QT Creator `sudo apt-get install qtcreator qtmultimedia5-dev`
1. Install boost-dev  `sudo apt-get install libboost-all-dev`
1. Copy `include/rapidjson` from [rapidjson](https://github.com/miloyip/rapidjson/releases/latest) into the chatterino directory
1. Open `chatterino.pro` with QT Creator and build

### Mac OSX
1. Install XCode and XCode Command Line Utilites
2. Install QT Creator
3. Install brew https://brew.sh/
4. `brew install boost openssl rapidjson`
5. Build the garbage using QT creator
6. gachiGASM
