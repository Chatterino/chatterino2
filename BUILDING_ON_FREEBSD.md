# FreeBSD

For all dependencies below we use Qt 6. Our minimum supported version is Qt 5.15.2, but you are on your own.

## FreeBSD 14.0-RELEASE

Note: This is known to work on FreeBSD 14.0-RELEASE amd64. Chances are
high that this also works on older FreeBSD releases, architectures and
FreeBSD 15.0-SNAP.

1. Install build dependencies from package sources (or build from the
   ports tree): `# pkg install boost-libs git libnotify qt6-base qt6-svg qt6-5compat qt6-imageformats qtkeychain-qt6 cmake`
1. In the project directory, create a build directory and enter it
   ```sh
   mkdir build
   cd build
   ```
1. Generate build files. To enable Lua plugins in your build add `-DCHATTERINO_PLUGINS=ON` to this command. Generating JSON implementation of EventSub blobs is slow (something to do with Python), if you're impatient you can enable the `-DSKIP_JSON_GENERATION=On` option.
   ```sh
   cmake ..
   ```
1. Build the project
   ```sh
   make
   ```
