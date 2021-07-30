freebsd {
    INCLUDEPATH += /usr/local/include/qt6keychain
    LIBS += -lqt6keychain
} else {
    unix:!android:!macx:!ios {
        DEFINES += KEYCHAIN_DBUS
    }
   include(qtkeychain/qt6keychain.pri)
}
