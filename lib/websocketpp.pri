# websocketpp
# Chatterino2 is tested with websocketpp 0.8.1
# Exposed build flags:
#  - WEBSOCKETPP_PREFIX ($$PWD by default)
#  - WEBSOCKETPP_SYSTEM (1 = true) (unix only)

!defined(WEBSOCKETPP_PREFIX) {
    WEBSOCKETPP_PREFIX = $$PWD
}

unix {
    equals(WEBSOCKETPP_SYSTEM, "1") {
        message("Building with system websocketpp")
    } else {
        message("Building with websocketpp submodule (Prefix: $$WEBSOCKETPP_PREFIX)")
        INCLUDEPATH += $$WEBSOCKETPP_PREFIX/websocketpp/
    }
} else {
    INCLUDEPATH += $$WEBSOCKETPP_PREFIX/websocketpp
}
