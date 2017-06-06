#-------------------------------------------------
#
# Project created by QtCreator 2016-12-28T18:23:35
#
#-------------------------------------------------

QT      += core gui network
CONFIG  += communi
COMMUNI += core model util
CONFIG  += c++14

DEFINES += IRC_NAMESPACE=Communi
include(lib/libcommuni/src/src.pri)

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# Include ourself
INCLUDEPATH += src/

TARGET   = chatterino
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

# Define warning flags for Chatterino
win32-msvc* {
    QMAKE_CXXFLAGS_WARN_ON = -W4
    # 4714 - function marked as __forceinline not inlined
    # 4996 - occurs when the compiler encounters a function or variable that is marked as deprecated.
    #        These functions may have a different preferred name, may be insecure or have
    #        a more secure variant, or may be obsolete.
    QMAKE_CXXFLAGS_WARN_ON += /wd4714
    QMAKE_CXXFLAGS_WARN_ON += /wd4996
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
    src/channel.cpp \
    src/colorscheme.cpp \
    src/emojis.cpp \
    src/ircmanager.cpp \
    src/messages/lazyloadedimage.cpp \
    src/messages/link.cpp \
    src/messages/message.cpp \
    src/messages/word.cpp \
    src/messages/wordpart.cpp \
    src/resources.cpp \
    src/widgets/chatwidget.cpp \
    src/widgets/chatwidgetheader.cpp \
    src/widgets/chatwidgetheaderbutton.cpp \
    src/widgets/chatwidgetinput.cpp \
    src/widgets/chatwidgetview.cpp \
    src/widgets/mainwindow.cpp \
    src/widgets/notebook.cpp \
    src/widgets/notebookbutton.cpp \
    src/widgets/notebookpage.cpp \
    src/widgets/notebookpagedroppreview.cpp \
    src/widgets/notebooktab.cpp \
    src/widgets/scrollbar.cpp \
    src/widgets/scrollbarhighlight.cpp \
    src/widgets/settingsdialog.cpp \
    src/widgets/settingsdialogtab.cpp \
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
    src/widgets/fancybutton.cpp \
    src/widgets/titlebar.cpp \
    src/appdatapath.cpp \
    src/accountmanager.cpp \
    src/twitch/twitchuser.cpp \
    src/ircaccount.cpp \
    src/widgets/accountpopup.cpp

HEADERS  += \
    src/asyncexec.h \
    src/channel.h \
    src/colorscheme.h \
    src/common.h \
    src/concurrentmap.h \
    src/emojis.h \
    src/ircmanager.h \
    src/messages/lazyloadedimage.h \
    src/messages/link.h \
    src/messages/message.h \
    src/messages/word.h \
    src/messages/wordpart.h \
    src/resources.h \
    src/setting.h \
    src/twitch/emotevalue.h \
    src/widgets/chatwidget.h \
    src/widgets/chatwidgetheader.h \
    src/widgets/chatwidgetheaderbutton.h \
    src/widgets/chatwidgetinput.h \
    src/widgets/chatwidgetview.h \
    src/widgets/mainwindow.h \
    src/widgets/notebook.h \
    src/widgets/notebookbutton.h \
    src/widgets/notebookpage.h \
    src/widgets/notebookpagedroppreview.h \
    src/widgets/notebooktab.h \
    src/widgets/scrollbar.h \
    src/widgets/scrollbarhighlight.h \
    src/widgets/settingsdialog.h \
    src/widgets/settingsdialogtab.h \
    src/widgets/signallabel.h \
    src/widgets/textinputdialog.h \
    src/widgets/resizingtextedit.h \
    src/settingssnapshot.h \
    src/messages/limitedqueue.h \
    src/messages/limitedqueuesnapshot.h \
    src/messages/messageref.h \
    src/logging/loggingmanager.h \
    src/logging/loggingchannel.h \
    src/channelmanager.h \
    src/windowmanager.h \
    src/settingsmanager.h \
    src/fontmanager.h \
    src/emotemanager.h \
    src/util/urlfetch.h \
    src/messages/messageparseargs.h \
    src/messages/messagebuilder.h \
    src/twitch/twitchmessagebuilder.h \
    src/twitch/twitchparsemessage.h \
    src/widgets/fancybutton.h \
    src/widgets/titlebar.h \
    src/appdatapath.h \
    src/accountmanager.h \
    src/twitch/twitchuser.h \
    src/ircaccount.h \
    src/widgets/accountpopup.h \
    src/util/distancebetweenpoints.h

PRECOMPILED_HEADER =

RESOURCES += \
    resources/resources.qrc

DISTFILES +=

# Include boost
win32 {
    INCLUDEPATH += C:\local\boost\
}

# Optional dependency on windows sdk 7.1
win32:exists(C:\Program Files\Microsoft SDKs\Windows\v7.1\Include\Windows.h) {
    LIBS += -L"C:\Program Files\Microsoft SDKs\Windows\v7.1\Lib" \
        -ldwmapi \
        -lgdi32

    SOURCES += platform/borderless/qwinwidget.cpp \
        platform/borderless/winnativewindow.cpp \
        platform/borderless/widget.cpp

    HEADERS += platform/borderless/qwinwidget.h \
        platform/borderless/winnativewindow.h \
        platform/borderless/widget.h

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
