export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/qt512/lib/
ldd ./bin/chatterino
make INSTALL_ROOT=appdir -j$(nproc) install ; find appdir/
cp ./resources/icon.png ./appdir/chatterino.png
wget -nv "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
chmod a+x linuxdeployqt-continuous-x86_64.AppImage
./linuxdeployqt-continuous-x86_64.AppImage appdir/usr/share/applications/*.desktop -no-translations -bundle-non-qt-libs -unsupported-allow-new-glibc -appimage -qmake=/opt/qt512/bin/qmake
mv Chatterino-*-x86_64.AppImage Chatterino-x86_64.AppImage
