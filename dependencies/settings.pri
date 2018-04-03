# settings
DEFINES += PAJLADA_SETTINGS_NO_BOOST

SOURCES += \
           $$PWD/../lib/settings/src/settings/settingdata.cpp \
           $$PWD/../lib/settings/src/settings/settingmanager.cpp

INCLUDEPATH += $$PWD/../lib/settings/include/
