win32 {
    INCLUDEPATH += C:/local/openssl/include

    LIBS += -LC:\local\openssl\lib

    LIBS += -llibssl
    LIBS += -llibcrypto
} else {
    PKGCONFIG += openssl

    LIBS += -lssl -lcrypto
}
