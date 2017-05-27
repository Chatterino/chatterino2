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

SOURCES += main.cpp\
    channel.cpp \
    colorscheme.cpp \
    emojis.cpp \
    ircmanager.cpp \
    messages/lazyloadedimage.cpp \
    messages/link.cpp \
    messages/message.cpp \
    messages/word.cpp \
    messages/wordpart.cpp \
    resources.cpp \
    widgets/chatwidget.cpp \
    widgets/chatwidgetheader.cpp \
    widgets/chatwidgetheaderbutton.cpp \
    widgets/chatwidgetinput.cpp \
    widgets/chatwidgetview.cpp \
    widgets/mainwindow.cpp \
    widgets/notebook.cpp \
    widgets/notebookbutton.cpp \
    widgets/notebookpage.cpp \
    widgets/notebookpagedroppreview.cpp \
    widgets/notebooktab.cpp \
    widgets/scrollbar.cpp \
    widgets/scrollbarhighlight.cpp \
    widgets/settingsdialog.cpp \
    widgets/settingsdialogtab.cpp \
    widgets/textinputdialog.cpp \
    messages/messageref.cpp \
    logging/loggingmanager.cpp \
    logging/loggingchannel.cpp \
    windowmanager.cpp \
    channelmanager.cpp \
    fontmanager.cpp \
    settingsmanager.cpp \
    emotemanager.cpp \
    messages/messagebuilder.cpp \
    twitch/twitchmessagebuilder.cpp \
    twitch/twitchparsemessage.cpp \
    widgets/fancybutton.cpp \
    widgets/titlebar.cpp \
    appdatapath.cpp \
    accountmanager.cpp \
    twitch/twitchaccount.cpp \
    ircaccount.cpp \
    widgets/accountpopup.cpp

HEADERS  += \
    asyncexec.h \
    channel.h \
    colorscheme.h \
    common.h \
    concurrentmap.h \
    emojis.h \
    ircmanager.h \
    messages/lazyloadedimage.h \
    messages/link.h \
    messages/message.h \
    messages/word.h \
    messages/wordpart.h \
    resources.h \
    setting.h \
    twitch/emotevalue.h \
    widgets/chatwidget.h \
    widgets/chatwidgetheader.h \
    widgets/chatwidgetheaderbutton.h \
    widgets/chatwidgetinput.h \
    widgets/chatwidgetview.h \
    widgets/mainwindow.h \
    widgets/notebook.h \
    widgets/notebookbutton.h \
    widgets/notebookpage.h \
    widgets/notebookpagedroppreview.h \
    widgets/notebooktab.h \
    widgets/scrollbar.h \
    widgets/scrollbarhighlight.h \
    widgets/settingsdialog.h \
    widgets/settingsdialogtab.h \
    widgets/signallabel.h \
    widgets/textinputdialog.h \
    widgets/resizingtextedit.h \
    settingssnapshot.h \
    messages/limitedqueue.h \
    messages/limitedqueuesnapshot.h \
    messages/messageref.h \
    logging/loggingmanager.h \
    logging/loggingchannel.h \
    channelmanager.h \
    windowmanager.h \
    settingsmanager.h \
    fontmanager.h \
    emotemanager.h \
    util/urlfetch.h \
    messages/messageparseargs.h \
    messages/messagebuilder.h \
    twitch/twitchmessagebuilder.h \
    twitch/twitchparsemessage.h \
    widgets/fancybutton.h \
    widgets/titlebar.h \
    appdatapath.h \
    accountmanager.h \
    twitch/twitchaccount.h \
    ircaccount.h \
    widgets/accountpopup.h \
    util/distancebetweenpoints.h

PRECOMPILED_HEADER =

RESOURCES += \
    resources.qrc

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
