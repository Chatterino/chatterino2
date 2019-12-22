# rapidjson
# Chatterino2 is tested with RapidJSON v1.1.0
#  - RAPIDJSON_PREFIX ($$PWD by default)
#  - RAPIDJSON_SYSTEM (1 = true) (Linux only, uses pkg-config)

!defined(RAPIDJSON_PREFIX) {
    RAPIDJSON_PREFIX = $$PWD
}

linux:equals(RAPIDJSON_SYSTEM, "1") {
    message("Building with system RapidJSON")
    PKGCONFIG += RapidJSON
} else {
    INCLUDEPATH += $$RAPIDJSON_PREFIX/rapidjson/include/
}
