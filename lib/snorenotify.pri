# Requires cmake doxygen extra-cmake-modules
!exists($$PWD/snorenotify/bin) {
    linux {
        CMAKE_SNORENOTIFY = $$system("cmake snorenotify/CMakeLists.txt")
        REMOVE_LINE = $$system("sed -i '/get_git_head_revision(GIT_REFSPEC SNORE_REVISION)/d' snorenotify/src/libsnore/CMakeLists.txt")
        MAKE_SNORENOTIFY = $$system("make -C snorenotify/")
    } win-32 {
        # Haven't tested
        CMAKE_SNORENOTIFY = $$system("cmake snorenotify/CMakeLists.txt")
        MAKE_SNORENOTIFY = $$system("nmake snorenotify/Makefile")
    }

}
LIBS += -L../lib/snorenotify/bin -lsnore-qt5
LIBS += -L../lib/snorenotify/bin -lsnoresettings-qt5
INCLUDEPATH += $$PWD/snorenotify/src
