# Define warning flags for Chatterino
win32-msvc* {
    QMAKE_CXXFLAGS_WARN_ON = /W4
    # 4714 - function marked as __forceinline not inlined
    # 4996 - occurs when the compiler encounters a function or variable that is marked as deprecated.
    #        These functions may have a different preferred name, may be insecure or have
    #        a more secure variant, or may be obsolete.
    # 4505 - unreferenced local version has been removed
    # 4127 - conditional expression is constant
    # 4503 - decorated name length exceeded, name was truncated
    # 4100 - unreferences formal parameter
    # 4305 - possible truncation of data
    # 4267 - possible loss of data in return
    QMAKE_CXXFLAGS_WARN_ON += /wd4714
    QMAKE_CXXFLAGS_WARN_ON += /wd4996
    QMAKE_CXXFLAGS_WARN_ON += /wd4505
    QMAKE_CXXFLAGS_WARN_ON += /wd4127
    QMAKE_CXXFLAGS_WARN_ON += /wd4503
    QMAKE_CXXFLAGS_WARN_ON += /wd4100
    QMAKE_CXXFLAGS_WARN_ON += /wd4305
    QMAKE_CXXFLAGS_WARN_ON += /wd4267

} else {
    QMAKE_CXXFLAGS_WARN_ON = -Wall
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-function
    QMAKE_CXXFLAGS_WARN_ON += -Wno-switch
    QMAKE_CXXFLAGS_WARN_ON += -Wno-deprecated-declarations
    QMAKE_CXXFLAGS_WARN_ON += -Wno-sign-compare
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-variable

    # Disabling strict-aliasing warnings for now, although we probably want to re-enable this in the future
    QMAKE_CXXFLAGS_WARN_ON += -Wno-strict-aliasing

    QMAKE_CXXFLAGS_WARN_ON += -Werror=return-type

    CXX_VERSION = $$system($$QMAKE_CXX --version)
    contains(CXX_VERSION, "clang") {
        QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-local-typedef
        QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-private-field
    } else {
        QMAKE_CXXFLAGS_WARN_ON += -Wno-class-memaccess
    }
}
