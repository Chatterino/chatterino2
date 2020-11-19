# FreeBSD

Note on Qt version compatibility: If you are installing Qt from a package manager, please ensure the version you are installing is at least **Qt 5.12 or newer**.

## FreeBSD 12.1-RELEASE

Note: This is known to work on FreeBSD 12.1-RELEASE amd64. Chances are
high that this also works on older FreeBSD releases, architectures and
FreeBSD 13.0-CURRENT.

1. Install build dependencies from package sources (or build from the
   ports tree): `# pkg install qt5-core qt5-multimedia qt5-svg
   qt5-qmake qt5-buildtools gstreamer-plugins-good boost-libs
   rapidjson`
1. go into project directory
1. create build folder `$ mkdir build && cd build`
1. `$ qmake .. && make`
