# settings
DEFINES += PAJLADA_SETTINGS_BOOST_FILESYSTEM

SOURCES += \
           $$PWD/settings/src/settings/settingdata.cpp \
           $$PWD/settings/src/settings/settingmanager.cpp \
           $$PWD/settings/src/settings/detail/realpath.cpp

INCLUDEPATH += $$PWD/settings/include/
