pajlada {
    BOOST_DIRECTORY = C:\dev\projects\boost_1_66_0\
}

win32 {
    isEmpty(BOOST_DIRECTORY) {
        message(Using default boost directory C:\\local\\boost\\)
        BOOST_DIRECTORY = C:\local\boost\
    }

    INCLUDEPATH += $$BOOST_DIRECTORY

    LIBS += -L$$BOOST_DIRECTORY\lib
}
