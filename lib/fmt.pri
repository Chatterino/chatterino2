# fmt
# Chatterino2 is tested with FMT 4.0
# Exposed build flags:
#  - FMT_PREFIX ($$PWD by default)
#  - FMT_SYSTEM (1 = true) (Linux only, uses pkg-config)

!defined(FMT_PREFIX) {
    FMT_PREFIX = $$PWD
}

linux:equals(FMT_SYSTEM, "1") {
    message("Building with system FMT")
    PKGCONFIG += fmt
} else {
    SOURCES += $$FMT_PREFIX/fmt/fmt/format.cpp

    INCLUDEPATH += $$PWD/fmt/
}
