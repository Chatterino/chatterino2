# Building on Windows

**Note that installing all of the development prerequisites and libraries will require about 30 GB of free disk space. Please ensure this space is available on your `C:` drive before proceeding.**

This guide assumes you are on a 64-bit system. You might need to manually search out alternate download links should you desire to build Chatterino on a 32-bit system.

## Visual Studio 2019

Download and install [Visual Studio 2019 Community](https://visualstudio.microsoft.com/downloads/). In the installer, select "Desktop development with C++" and "Universal Windows Platform development".

Notes:

- This installation will take about 17 GB of disk space
- You do not need to sign in with a Microsoft account after setup completes. You may simply exit the login dialog.

## Boost

1. First, download a boost installer appropriate for your version of Visual Studio.

   - Visit the downloads list on [SourceForge](https://sourceforge.net/projects/boost/files/boost-binaries/).
   - Select the latest version from the list.
   - Download the `.exe` file appropriate to your Visual Studio installation version and system bitness (choose `-64` for 64-bit systems).
     Visual Studio versions map as follows: `14.2` in the filename corresponds to MSVC 2019, `14.1` to 2017, `14.0` to 2015. _Anything prior to Visual Studio 2015 is unsupported. Please upgrade should you have an older installation._

     **Convenience link for Visual Studio 2019: [boost_1_76_0-msvc-14.2-64.exe](https://sourceforge.net/projects/boost/files/boost-binaries/1.76.0/boost_1_76_0-msvc-14.2-64.exe/download)**

2. When prompted where to install Boost, set the location to `C:\local\boost`.
3. After the installation finishes, rename the `C:\local\boost\lib64-msvc-14.2` (or similar) directory to simply `lib` (`C:\local\boost\lib`).

Note: This installation will take about 1.5 GB of disk space.

## OpenSSL

### For our websocket library, we need OpenSSL 1.1

1. Download OpenSSL for windows, version `1.1.1k`: **[Download](https://slproweb.com/download/Win64OpenSSL-1_1_1k.exe)**
2. When prompted, install OpenSSL to `C:\local\openssl`
3. When prompted, copy the OpenSSL DLLs to "The OpenSSL binaries (/bin) directory".

### For Qt SSL, we need OpenSSL 1.0

1. Download OpenSSL for Windows, version `1.0.2u`: **[Download](https://slproweb.com/download/Win64OpenSSL-1_0_2u.exe)**
2. When prompted, install it to any arbitrary empty directory.
3. When prompted, copy the OpenSSL DLLs to "The OpenSSL binaries (/bin) directory".
4. Copy the OpenSSL 1.0 files from its `\bin` folder to `C:\local\bin` (You will need to create the folder)
5. Then copy the OpenSSL 1.1 files from its `\bin` folder to `C:\local\bin` (Overwrite any duplicate files)
6. Add `C:\local\bin` to your path folder ([Follow the guide here if you don't know how to do it](https://www.computerhope.com/issues/ch000549.htm#windows10))

**If the download links above do not work, try downloading similar 1.1.x & 1.0.x versions [here](https://slproweb.com/products/Win32OpenSSL.html). Note: Don't download the "light" installers, they do not have the required files.**

Note: This installation will take about 200 MB of disk space.

## Qt

1. Visit the [Qt Open Source Page](https://www.qt.io/download-open-source).
2. Scroll down to the bottom
3. Then select "Download the Qt Online Installer"

Notes:

- Installing the latest **stable** Qt version is advised for new installations, but if you want to use your existing installation please ensure you are running **Qt 5.12 or later**.

### When prompted which components to install:

1. Unfold the tree element that says "Qt"
2. Unfold the top most tree element (latest stable Qt version, e.g. `Qt 5.15.2`)
3. Under this version, select the following entries:
   - `MSVC 2019 64-bit` (or alternative version if you are using that)
   - `Qt WebEngine` (optional)
4. Under the "Tools" tree element (at the bottom), ensure that `Qt Creator X.X.X` and `Qt Creator X.X.X CDB Debugger Support` are selected. (they should be checked by default)
5. Continue through the installer and let the installer finish installing Qt.

Note: This installation will take about 2 GB of disk space.

## Compile with Breakpad support (Optional)

Compiling with Breakpad support enables crash reports that can be of use for developing/beta versions of Chatterino. If you have no interest in reporting crashes anyways, this optional dependency will probably be of no use to you.

1. Open up `lib/qBreakpad/handler/handler.pro`in Qt Creator
2. Build it in whichever mode you want to build Chatterino in (Debug/Profile/Release)
3. Copy the newly built `qBreakpad.lib` to the following directory: `lib/qBreakpad/build/handler` (You will have to manually create this directory)

## Run the build in Qt Creator

1. Open the `chatterino.pro` file by double-clicking it, or by opening it via Qt Creator.
2. You will be presented with a screen that is titled "Configure Project". In this screen, you should have at least one option present ready to be configured, like this:
   ![Qt Create Configure Project screenshot](https://i.imgur.com/dbz45mB.png)
3. Select the profile(s) you want to build with and click "Configure Project".

### How to run and produce builds

- In the main screen, click the green "play symbol" on the bottom left to run the project directly.
- Click the hammer on the bottom left to generate a build (does not run the build though).

Build results will be placed in a folder at the same level as the "chatterino2" project folder (e.g. if your sources are at `C:\Users\example\src\chatterino2`, then the build will be placed in an automatically generated folder under `C:\Users\example\src`, e.g. `C:\Users\example\src\build-chatterino-Desktop_Qt_5_15_2_MSVC2019_64bit-Release`.)

- Note that if you are building chatterino purely for usage, not for development, it is recommended that you click the "PC" icon above the play icon and select "Release" instead of "Debug".
- Output and error messages produced by the compiler can be seen under the "4 Compile Output" tab in Qt Creator.

## Producing standalone builds

If you build chatterino, the result directories will contain a `chatterino.exe` file in the `$OUTPUTDIR\release\` directory. This `.exe` file will not directly run on any given target system, because it will be lacking various Qt runtimes.

To produce a standalone package, you need to generate all required files using the tool `windeployqt`. This tool can be found in the `bin` directory of your Qt installation, e.g. at `C:\Qt\5.15.2\msvc2019_64\bin\windeployqt.exe`.

To produce all supplement files for a standalone build, follow these steps (adjust paths as required):

1.  Navigate to your build output directory with Windows Explorer, e.g. `C:\Users\example\src\build-chatterino-Desktop_Qt_5_15_2_MSVC2019_64bit-Release`
2.  Enter the `release` directory
3.  Delete all files except the `chatterino.exe` file. You should be left with a directory only containing `chatterino.exe`.
4.  Open a command prompt and execute:

        cd C:\Users\example\src\build-chatterino-Desktop_Qt_5_15_2_MSVC2019_64bit-Release\release
        C:\Qt\5.15.2\msvc2019_64\bin\windeployqt.exe chatterino.exe

5.  Go to `C:\local\bin\` and copy these dll's into your `release folder`.

    libssl-1_1-x64.dll
    libcrypto-1_1-x64.dll
    ssleay32.dll
    libeay32.dll

6.  The `releases` directory will now be populated with all the required files to make the chatterino build standalone.

You can now create a zip archive of all the contents in `releases` and distribute the program as is, without requiring any development tools to be present on the target system. (However, the vcredist package must be present, as usual - see the [README](README.md)).

## Using CMake

Open up your terminal with the Visual Studio environment variables, then enter the following commands:

1. `mkdir build`
2. `cd build`
3. `cmake -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DUSE_CONAN=ON ..`
4. `nmake`
