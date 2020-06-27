![alt text](https://fourtf.com/img/chatterino-icon-64.png)
Chatterino 2
============

Chatterino 2 is the second installment of the Twitch chat client series "Chatterino".

## Download
Current releases are available at [https://chatterino.com](https://chatterino.com).

## Nightly build
You can download the latest Chatterino 2 build over [here](https://github.com/Chatterino/chatterino2/releases/tag/nightly-build)

You might also need to install the [VC++ 2017 Redistributable](https://aka.ms/vs/15/release/vc_redist.x64.exe) from Microsoft if you do not have it installed already.
If you still receive an error about `MSVCR120.dll missing`, then you should install the [VC++ 2013 Restributable](https://download.microsoft.com/download/2/E/6/2E61CFA4-993B-4DD4-91DA-3737CD5CD6E3/vcredist_x64.exe
).

## Building
To get source code with required submodules run:
```
git clone --recurse-submodules https://github.com/Chatterino/chatterino2.git
```
or
```
git clone https://github.com/Chatterino/chatterino2.git
cd chatterino2
git submodule update --init --recursive
```

[Building on Windows](../master/BUILDING_ON_WINDOWS.md)

[Building on Linux](../master/BUILDING_ON_LINUX.md)

[Building on Mac](../master/BUILDING_ON_MAC.md)

[Building on FreeBSD](../master/BUILDING_ON_FREEBSD.md)

## Code style
The code is formatted using clang format in Qt Creator. [.clang-format](src/.clang-format) contains the style file for clang format.

### Get it automated with QT Creator + Beautifier + Clang Format
1. Download LLVM: https://releases.llvm.org/9.0.0/LLVM-9.0.0-win64.exe
2. During the installation, make sure to add it to your path
3. In QT Creator, select `Help` > `About Plugins` > `C++` > `Beautifier` to enable the plugin
4. Restart QT Creator
5. Select `Tools` > `Options` > `Beautifier`
6. Under `General` select `Tool: ClangFormat` and enable `Automatic Formatting on File Save`
7. Under `Clang Format` select `Use predefined style: File` and `Fallback style: None`

Qt creator should now format the documents when saving it.

## Doxygen
Doxygen is used to generate project information daily and is available [here](https://doxygen.chatterino.com).
