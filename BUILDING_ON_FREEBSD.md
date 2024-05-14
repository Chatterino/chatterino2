# FreeBSD

For all dependencies below we use Qt 6. Our minimum supported version is Qt 5.15.2, but you are on your own.

## FreeBSD 14.0-RELEASE

Note: This is known to work on FreeBSD 14.0-RELEASE amd64. Chances are
high that this also works on older FreeBSD releases, architectures and
FreeBSD 15.0-SNAP.

1. Install build dependencies from package sources (or build from the
   ports tree): `# pkg install boost-libs git qt6-base qt6-svg qtkeychain-qt6 cmake`
1. Generate build files. To enable Lua plugins in your build add `-DCHATTERINO_PLUGINS=ON` to this command.
   ```sh
   cmake -B build
   ```
1. Build the project
   ```sh
   make -C build
   ```
