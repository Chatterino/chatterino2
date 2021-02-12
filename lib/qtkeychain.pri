freebsd {
    INCLUDEPATH += /usr/local/include
    LIBS += -lqt5keychain
} else {
    unix:!android:!macx:!ios {
        DEFINES += KEYCHAIN_DBUS
    }
   include(qtkeychain/qt5keychain.pri)
}
