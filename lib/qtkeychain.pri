freebsd {
    INCLUDEPATH += /usr/local/include/qt5keychain
    LIBS += -lqt5keychain
} else {
   include(qtkeychain/qt5keychain.pri)
}
