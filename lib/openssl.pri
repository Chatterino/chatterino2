win32 {
    isEmpty(OPENSSL_DIRECTORY) {
        message(Using default openssl directory C:\\local\\openssl)
        OPENSSL_DIRECTORY = C:\local\openssl
    }

    INCLUDEPATH += $$OPENSSL_DIRECTORY\\include

    LIBS += -L$$OPENSSL_DIRECTORY\lib

    LIBS += -llibssl
    LIBS += -llibcrypto
} else {
    PKGCONFIG += openssl

    LIBS += -lssl -lcrypto
}
