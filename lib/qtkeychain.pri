freebsd {
    INCLUDEPATH += /usr/local/include/qt5keychain
    LIBS += -Lqt5keychain
} else {
   include(qtkeychain/qt5keychain.pri)
}
