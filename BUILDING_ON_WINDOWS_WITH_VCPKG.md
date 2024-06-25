# Building on Windows with vcpkg

This will require more than 30 GB of free space on your hard drive.

## Prerequisites

1. Install [Visual Studio](https://visualstudio.microsoft.com/) with "Desktop development with C++"
1. Install [CMake](https://cmake.org/)
1. Install [git](https://git-scm.com/)
1. Install [vcpkg](https://vcpkg.io/)

   ```shell
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg integrate install
   .\vcpkg integrate powershell
   cd ..
   ```

1. Configure the environment variables for vcpkg.  
    Check [this document](https://gist.github.com/mitchmindtree/92c8e37fa80c8dddee5b94fc88d1288b#setting-an-environment-variable-on-windows) for more information for how to set environment variables on Windows.
   - Ensure your dependencies are built as 64-bit  
     e.g. `setx VCPKG_DEFAULT_TRIPLET x64-windows`  
     See [documentation about Triplets](https://learn.microsoft.com/en-gb/vcpkg/users/triplets)  
     [default](https://github.com/microsoft/vcpkg/blob/master/docs/users/triplets.md#additional-remarks) is `x86-windows`
   - Set VCPKG_ROOT to the vcpkg path  
     e.g. `setx VCPKG_ROOT <path to vcpkg>`  
     See [VCPKG_ROOT documentation](https://learn.microsoft.com/en-gb/vcpkg/users/config-environment#vcpkg_root)
   - Append the vcpkg path to your path  
     e.g. `setx PATH "%PATH%;<path to vcpkg>"`
   - For more configurations, see <https://learn.microsoft.com/en-gb/vcpkg/users/config-environment>
1. You may need to restart your computer to ensure all your environment variables and what-not are loaded everywhere.

## Building

1. Clone
   ```shell
   git clone --recurse-submodules https://github.com/Chatterino/chatterino2.git
   ```
1. Install dependencies
   ```powershell
   cd .\chatterino2\
   vcpkg install
   ```
1. Build
   ```powershell
   cmake -B build -DCMAKE_TOOLCHAIN_FILE="$Env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
   cd build
   cmake --build . --parallel <threads> --config Release
   ```
   When using CMD, use `-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake` to specify the toolchain.
   To build with plugins add `-DCHATTERINO_PLUGINS=ON` to `cmake -B build` command.
1. Run `.\bin\chatterino2.exe`
