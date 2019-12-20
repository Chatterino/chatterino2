# rapidjson
# Chatterino2 is tested with RapidJSON v1.1.0
#  - RAPIDJSON_PREFIX ($$PWD by default)
#  - RAPIDJSON_SYSTEM (1 = true) (unix only)

!defined(RAPIDJSON_PREFIX) {
    RAPIDJSON_PREFIX = $$PWD
}
unix {

}
unix {
    equals(RAPIDJSON_SYSTEM, "1") {
        message("Building with system RapidJSON")
    } else {
        message("Building with RapidJSON submodule (Prefix: $$RAPIDJSON_PREFIX)")

        INCLUDEPATH += $$RAPIDJSON_PREFIX/rapidjson/include/
    }
} else {
    INCLUDEPATH += $$RAPIDJSON_PREFIX/rapidjson/include/
}
