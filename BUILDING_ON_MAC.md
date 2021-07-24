# Building on macOS

#### Note - If you want to develop Chatterino 2 you will also need to install Qt Creator (make sure to install **Qt 5.12 or newer**)

#### Note - Chatterino 2 is only tested on macOS 10.14 and above - anything below that is considered unsupported. It may or may not work on earlier versions

1. Install Xcode and Xcode Command Line Utilities
2. Start Xcode, go into Settings -> Locations, and activate your Command Line Tools
3. Install brew https://brew.sh/
4. Install the dependencies using `brew install boost openssl rapidjson`
5. Install Qt using `brew install qt`
6. Step 5 should output some directions to add Qt to your path, you will need to do this for qmake
7. Go into the project directory
8. Create a build folder and go into it (`mkdir build && cd build`)
9. Compile using `qmake .. && make`

_If you want to use cmake instead of qmake, just replace the above qmake command with cmake_

If the Project does not build at this point, you might need to add additional Paths/Libs, because brew does not install openssl and boost in the common path. You can get their path using

`brew info openssl`
`brew info boost`

If brew doesn't link OpenSSL properly then you should be able to link it yourself by using these two commands:

- `ln -s /usr/local/opt/openssl/lib/* /usr/local/lib`
- `ln -s /usr/local/opt/openssl/include/openssl /usr/local/include/openssl`

The lines which you need to add to your project file should look similar to this

```
INCLUDEPATH += /usr/local/opt/openssl/include
LIBS += -L/usr/local/opt/openssl/lib
```
