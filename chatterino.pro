#-------------------------------------------------
#
# Project created by QtCreator 2016-12-28T18:23:35
#
#-------------------------------------------------

QT      += core gui network multimedia
CONFIG  += communi
COMMUNI += core model util
CONFIG  += c++14
PRECOMPILED_HEADER = precompiled_header.hpp

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# Include ourself
INCLUDEPATH += src/

TARGET   = chatterino
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

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
    QMAKE_CXXFLAGS_WARN_ON += /wd4714
    QMAKE_CXXFLAGS_WARN_ON += /wd4996
    QMAKE_CXXFLAGS_WARN_ON += /wd4505
    QMAKE_CXXFLAGS_WARN_ON += /wd4127
    QMAKE_CXXFLAGS_WARN_ON += /wd4503

} else {
    QMAKE_CXXFLAGS_WARN_ON = -Wall
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-function
    QMAKE_CXXFLAGS_WARN_ON += -Wno-switch
    QMAKE_CXXFLAGS_WARN_ON += -Wno-deprecated-declarations
}

# do not use windows min/max macros
win32 {
    DEFINES += NOMINMAX
}

#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/main.cpp \
    src/application.cpp \
    src/channel.cpp \
    src/channeldata.cpp \
    src/colorscheme.cpp \
    src/ircmanager.cpp \
    src/messages/lazyloadedimage.cpp \
    src/messages/link.cpp \
    src/messages/message.cpp \
    src/messages/word.cpp \
    src/messages/wordpart.cpp \
    src/resources.cpp \
    src/widgets/notebook.cpp \
    src/widgets/helper/notebookbutton.cpp \
    src/widgets/helper/notebooktab.cpp \
    src/widgets/scrollbar.cpp \
    src/widgets/helper/scrollbarhighlight.cpp \
    src/widgets/settingsdialog.cpp \
    src/widgets/helper/settingsdialogtab.cpp \
    src/widgets/textinputdialog.cpp \
    src/messages/messageref.cpp \
    src/logging/loggingmanager.cpp \
    src/logging/loggingchannel.cpp \
    src/windowmanager.cpp \
    src/channelmanager.cpp \
    src/fontmanager.cpp \
    src/settingsmanager.cpp \
    src/emotemanager.cpp \
    src/messages/messagebuilder.cpp \
    src/twitch/twitchmessagebuilder.cpp \
    src/twitch/twitchparsemessage.cpp \
    src/widgets/titlebar.cpp \
    src/appdatapath.cpp \
    src/accountmanager.cpp \
    src/twitch/twitchuser.cpp \
    src/ircaccount.cpp \
    src/widgets/accountpopup.cpp \
    src/widgets/basewidget.cpp \
    src/widgets/helper/resizingtextedit.cpp \
    src/completionmanager.cpp \
    src/widgets/logindialog.cpp \
    src/widgets/qualitypopup.cpp \
    src/widgets/emotepopup.cpp \
    src/widgets/helper/channelview.cpp \
    src/twitch/twitchchannel.cpp \
    src/widgets/helper/rippleeffectlabel.cpp \
    src/widgets/helper/rippleeffectbutton.cpp \
    src/messages/messagecolor.cpp \
    src/util/networkmanager.cpp \
    src/commandmanager.cpp \
    src/widgets/split.cpp \
    src/widgets/helper/splitinput.cpp \
    src/widgets/helper/splitheader.cpp \
    src/widgets/splitcontainer.cpp \
    src/widgets/helper/droppreview.cpp \
    src/widgets/window.cpp \
    src/widgets/helper/splitcolumn.cpp \
    src/widgets/accountswitchwidget.cpp \
    src/widgets/accountswitchpopupwidget.cpp \
    src/widgets/tooltipwidget.cpp

HEADERS  += \
    src/precompiled_headers.hpp \
    src/asyncexec.hpp \
    src/channel.hpp \
    src/colorscheme.hpp \
    src/concurrentmap.hpp \
    src/emojis.hpp \
    src/ircmanager.hpp \
    src/messages/lazyloadedimage.hpp \
    src/messages/link.hpp \
    src/messages/message.hpp \
    src/messages/word.hpp \
    src/messages/wordpart.hpp \
    src/resources.hpp \
    src/setting.hpp \
    src/twitch/emotevalue.hpp \
    src/widgets/notebook.hpp \
    src/widgets/helper/notebookbutton.hpp \
    src/widgets/helper/notebooktab.hpp \
    src/widgets/scrollbar.hpp \
    src/widgets/helper/scrollbarhighlight.hpp \
    src/widgets/settingsdialog.hpp \
    src/widgets/helper/settingsdialogtab.hpp \
    src/widgets/helper/signallabel.hpp \
    src/widgets/textinputdialog.hpp \
    src/widgets/helper/resizingtextedit.hpp \
    src/settingssnapshot.hpp \
    src/messages/limitedqueue.hpp \
    src/messages/limitedqueuesnapshot.hpp \
    src/messages/messageref.hpp \
    src/logging/loggingmanager.hpp \
    src/logging/loggingchannel.hpp \
    src/channelmanager.hpp \
    src/windowmanager.hpp \
    src/settingsmanager.hpp \
    src/fontmanager.hpp \
    src/emotemanager.hpp \
    src/util/urlfetch.hpp \
    src/messages/messageparseargs.hpp \
    src/messages/messagebuilder.hpp \
    src/twitch/twitchmessagebuilder.hpp \
    src/twitch/twitchparsemessage.hpp \
    src/widgets/titlebar.hpp \
    src/appdatapath.hpp \
    src/accountmanager.hpp \
    src/twitch/twitchuser.hpp \
    src/ircaccount.hpp \
    src/widgets/accountpopup.hpp \
    src/util/distancebetweenpoints.hpp \
    src/widgets/basewidget.hpp \
    src/completionmanager.hpp \
    src/widgets/helper/channelview.hpp \
    src/twitch/twitchchannel.hpp \
    src/widgets/helper/rippleeffectbutton.hpp \
    src/widgets/helper/rippleeffectlabel.hpp \
    src/widgets/qualitypopup.hpp \
    src/widgets/emotepopup.hpp \
    src/messages/messagecolor.hpp \
    src/util/nativeeventhelper.hpp \
    src/debug/log.hpp \
    src/util/benchmark.hpp \
    src/util/networkmanager.hpp \
    src/commandmanager.hpp \
    src/widgets/split.hpp \
    src/widgets/helper/splitheader.hpp \
    src/widgets/helper/splitinput.hpp \
    src/widgets/window.hpp \
    src/widgets/splitcontainer.hpp \
    src/widgets/helper/droppreview.hpp \
    src/widgets/helper/splitcolumn.hpp \
    src/util/irchelpers.hpp \
    src/util/helpers.hpp \
    src/widgets/accountswitchwidget.hpp \
    src/widgets/accountswitchpopupwidget.hpp \
    src/const.hpp \
    src/widgets/tooltipwidget.hpp \
    src/precompiled_headers.hpp \
    src/messages/wordflags.hpp


PRECOMPILED_HEADER =

RESOURCES += \
    resources/resources.qrc

DISTFILES +=

# Include boost
win32 {
    INCLUDEPATH += C:\local\boost\
}

win32 {
    LIBS += -luser32
    LIBS += -lgdi32
}

# Optional dependency on windows sdk 7.1
win32:exists(C:\Program Files\Microsoft SDKs\Windows\v7.1\Include\Windows.h) {
    LIBS += -L"C:\Program Files\Microsoft SDKs\Windows\v7.1\Lib" \
        -ldwmapi \
        -lgdi32

#    SOURCES += platform/borderless/qwinwidget.cpp \
#        platform/borderless/winnativewindow.cpp \
#        platform/borderless/widget.cpp

#    HEADERS += platform/borderless/qwinwidget.h \
#        platform/borderless/winnativewindow.h \
#        platform/borderless/widget.h

    DEFINES += "USEWINSDK"
}

macx {
    INCLUDEPATH += /usr/local/include
}

FORMS += \
    forms/accountpopupform.ui

werr {
    QMAKE_CXXFLAGS += -Werror

    message("Enabling error on warning")
}

# External dependencies
include(dependencies/rapidjson.pri)
include(dependencies/settings.pri)
include(dependencies/signals.pri)
include(dependencies/humanize.pri)
include(dependencies/fmt.pri)
DEFINES += IRC_NAMESPACE=Communi
include(dependencies/libcommuni.pri)

