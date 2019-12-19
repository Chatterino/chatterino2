# boost
# Exposed build flags:
#  - BOOST_DIRECTORY (C:\local\boost\ by default) (Windows only)

pajlada {
    BOOST_DIRECTORY = C:\dev\projects\boost_1_66_0\
}

win32 {
    isEmpty(BOOST_DIRECTORY) {
        message(Using default boost directory C:\\local\\boost\\)
        BOOST_DIRECTORY = C:\local\boost\
    }

    INCLUDEPATH += $$BOOST_DIRECTORY

    isEmpty(BOOST_LIB_SUFFIX) {
        message(Using default boost lib directory suffix lib)
        BOOST_LIB_SUFFIX = lib
    }

    LIBS += -L$$BOOST_DIRECTORY\\$$BOOST_LIB_SUFFIX
} else {
    LIBS += -lboost_system -lboost_filesystem
}
