# fmt
# Chatterino2 is tested with FMT 4.0
# Exposed build flags:
#  - FMT_PREFIX ($$PWD by default)
#  - FMT_SYSTEM (1 = true) (unix only)

!defined(FMT_PREFIX) {
    FMT_PREFIX = $$PWD
}

unix {
    equals(FMT_SYSTEM, "1") {
        message("Building with system FMT")
        LIBS += -lfmt
    } else {
        message("Building with FMT submodule (Prefix: $$FMT_PREFIX)")
        SOURCES += $$FMT_PREFIX/fmt/fmt/format.cpp

        INCLUDEPATH += $$FMT_PREFIX/fmt/
    }
} else {
    SOURCES += $$FMT_PREFIX/fmt/fmt/format.cpp

    INCLUDEPATH += $$PWD/fmt/
}
