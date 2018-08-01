# Building on macOS
#### Note - If you want to develop Chatterino 2 you will also need to install Qt Creator
1. Install Xcode and Xcode Command Line Utilites
2. Start Xcode, settings -> Locations, activate your Command Line Tools
3. Install brew https://brew.sh/
4. `brew install boost openssl rapidjson`
5. `brew install qt`
6. Step 5 should output some directions to add qt to your path, you will need to do this for qmake
5. Go into project directory
6. Create build folder `mkdir build && cd build`
7. `qmake .. && make`

If the Project does not build at this point, you might need to add additional Paths/Libs, because brew does not install openssl and boost in the common path. You can get their path using

`brew info openssl`
`brew info boost`

If brew doesn't link openssl properly then you should be able to link it yourself using those two commands:
- `ln -s /usr/local/opt/openssl/lib/* /usr/local/lib`
- `ln -s /usr/local/opt/openssl/include/openssl /usr/local/include/openssl`

The lines which you need to add to your project file should look similar to this

```
INCLUDEPATH += /usr/local/opt/openssl/include
LIBS += -L/usr/local/opt/openssl/lib
```
