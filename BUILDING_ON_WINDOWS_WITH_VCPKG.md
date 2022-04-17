# Building on Windows with vcpkg

## Prerequisites

1. Install [Visual Studio](https://visualstudio.microsoft.com/) with "Desktop development with C++" (~9.66 GB)
1. Install [CMake](https://cmake.org/) (~109 MB)
1. Install [git](https://git-scm.com/) (~264 MB)
1. Install [vcpkg](https://vcpkg.io/) (~80 MB)
   - `git clone https://github.com/Microsoft/vcpkg.git`
   - `cd .\vcpkg\`
   - `.\bootstrap-vcpkg.bat`
   - `.\vcpkg integrate install`
   - `.\vcpkg integrate powershell`
   - `cd ..`
1. Configure the environment for vcpkg
   - `set VCPKG_DEFAULT_TRIPLET=x64-windows`
     - [default](https://github.com/microsoft/vcpkg/blob/master/docs/users/triplets.md#additional-remarks) is `x86-windows`
   - `set VCPKG_ROOT=C:\path\to\vcpkg\`
   - `set PATH=%PATH%;%VCPKG_ROOT%`

## Building

1. Clone
   - `git clone --recurse-submodules https://github.com/Chatterino/chatterino2.git`
1. Install dependencies (~21 GB)
   - `cd .\chatterino2\`
   - `vcpkg install`
1. Build
   - `mkdir .\build\`
   - `cd .\build\`
   - (cmd) `cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake`
   - (ps1) `cmake .. -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"`
   - `cmake --build . --parallel <threads> --config Release`
1. Run
   - `.\bin\chatterino2.exe`
