# Building on Windows (Recommended)
## Using Qt Creator
### Visual Studio 2017
1. Install Visual Studio 2017 and select "Desktop development with C++" and "Universal Windows Platform development.

### Boost
1. Visual Studio 2017 64-bit: https://dl.bintray.com/boostorg/release/1.66.0/binaries/boost_1_66_0-msvc-14.1-64.exe
2. When prompted, install boost to C:\local\boost
3. When the installation is finished, go to C:\local\boost and rename the "lib64-msvc-14.1" folder to "lib"

### OpenSSL
#### For our websocket library, we need OpenSSL 1.1
1. Download OpenSSL development library: https://slproweb.com/download/Win64OpenSSL-1_1_0h.exe
2. When prompted, install openssl to C:\local\openssl
3. When prompted, copy the OpenSSL DLLs to "The OpenSSL binaries (/bin) directory"
#### For Qt SSL, we need OpenSSL 1.0
1. Download OpenSSL light: https://slproweb.com/download/Win64OpenSSL_Light-1_0_2o.exe
2. When prompted, install it anywhere
3. When prompted, copy the OpenSSL DLLS to "The OpenSSL binaries (/bin) directory"
4. Copy the OpenSSL 1.0 files from its /bin folder to C:/local/bin (You will need to create the folder)
5. Then copy the OpenSSL 1.1 files from its /bin folder to C:/local/bin (Overwrite any duplicate files)
6. Add C:/local/bin to your path folder (Follow guide here if you don't know how to do it: https://www.computerhope.com/issues/ch000549.htm#windows8 )

### Qt
1. Download Qt: https://www.qt.io/download
2. Select "Open source" at the bottom of this page
3. Then select "Download"
#### When prompted which components to install:
1. Under the latest Qt version:
  - Select MSVC 2017 64-bit (or MSVC 2015 64-bit if you still use Visual Studio 2015)
  - Optionally, enable Qt WebEngine
2. Under Tools:
  - Select Qt Creator, and Qt Creator CDB Debugger Support

### Compile with Breakpad support (Optional)
1. Open up lib/qBreakpad/handler/handler.pro
2. Build it in whichever mode you wanna build Chatterino (Debug/Profile/Release)
3. Copy the newly built qBreakpad.lib to a folder you create: `lib/qBreakpad/build/handler`

# Windows (Using MSYS2, not recommended)
Note: This guide is currently out of date and will not work as is.
Note: This build will have some features missing from the build.

Building using MSYS2 can be quite easier process. Check out MSYS2 at [msys2.org](http://www.msys2.org/).

Be sure to add `-j <number of threads>` as a make argument so it will use all your cpu cores to build. [example setup](https://i.imgur.com/qlESlS1.png)

You can also add `-o2` to optimize the final binary size but increase compilation time, and add `-pipe` to use more ram in compilation but increase compilation speed
1. open appropriate MSYS2 terminal and do `pacman -S mingw-w64-<arch>-boost mingw-w64-<arch>-qt5 mingw-w64-<arch>-rapidjson` where `<arch>` is `x86_64` or `i686`
2. go into the project directory
3. create build folder `mkdir build && cd build`
4. `qmake .. && mingw32-make`
