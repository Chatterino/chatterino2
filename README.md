![alt text](https://fourtf.com/img/chatterino-icon-64.png)
Chatterino 2
============

Chatterino 2 is the second installment of the Twitch chat client series "Chatterino". For now you can check out Chatterino 1 at [https://chatterino.com](https://chatterino.com).

## Downloading
You can download the Chatterino 2 Beta over [here](https://chatterino.com/download/Chatterino2BetaInstaller.exe)

You might also need to install the [VC++ 2017 Redistributable](https://aka.ms/vs/15/release/vc_redist.x64.exe) from Microsoft if you do not have it installed already.  
If you still receive an error about `MSVCR120.dll missing`, then you should install the [VC++ 2017 Restributable](https://download.microsoft.com/download/2/E/6/2E61CFA4-993B-4DD4-91DA-3737CD5CD6E3/vcredist_x64.exe
).

Releases for linux and mac will follow soon™

## Building
Before building run `git submodule update --init --recursive` to get required submodules.

[Building on Windows](../master/BUILDING_ON_WINDOWS.md)

[Building on Linux](../master/BUILDING_ON_LINUX.md)

[Building on Mac](../master/BUILDING_ON_MAC.md)

## Code style
The code is formated using clang format in Qt Creator. [.clang-format](https://github.com/fourtf/chatterino2/blob/master/.clang-format) contains the style file for clang format.

### Get it automated with QT Creator + Beautifier + Clang Format
1. Download LLVM: http://releases.llvm.org/6.0.1/LLVM-6.0.1-win64.exe
2. During the installation, make sure to add it to your path
3. In QT Creator, select `Help` > `About Plugins` > `C++` > `Beautifier` to enable the plugin
4. Restart QT Creator
5. Select `Tools` > `Options` > `Beautifier`
6. Under `General` select `Tool: ClangFormat` and enable `Automatic Formatting on File Save`
7. Under `Clang Format` select `Use predefined style: File` and `Fallback style: None`

Qt creator should now format the documents when saving it.

### CodeScene
[![](https://codescene.io/projects/3004/status.svg) Get more details at **codescene.io**.](https://codescene.io/projects/3004/jobs/latest-successful/results)
