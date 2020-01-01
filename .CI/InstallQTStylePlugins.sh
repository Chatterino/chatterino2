git clone http://code.qt.io/qt/qtstyleplugins.git
cd qtstyleplugins
/opt/qt512/bin/qmake CONFIG+=release
make -j$(nproc)
sudo make install
cd -
