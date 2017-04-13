#-------------------------------------------------
#
# Project created by QtCreator 2016-12-28T18:23:35
#
#-------------------------------------------------

QT      += core gui network
CONFIG  += communi
COMMUNI += core model util
CONFIG  += c++14

include(lib/libcommuni/src/src.pri)

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET   = chatterino
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += IRC_NAMESPACE=Communi

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
    usermanager.cpp \
    twitch/twitchuser.cpp \
    messages/messagebuilder.cpp \
    twitch/twitchmessagebuilder.cpp \
    ircuser2.cpp \
    twitch/twitchparsemessage.cpp \
    widgets/fancybutton.cpp \
    widgets/titlebar.cpp \
    widgets/userpopupwidget.cpp \
    appdatapath.cpp

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
    usermanager.h \
    twitch/twitchuser.h \
    messages/messageparseargs.h \
    messages/messagebuilder.h \
    twitch/twitchmessagebuilder.h \
    ircuser2.h \
    twitch/twitchparsemessage.h \
    widgets/fancybutton.h \
    widgets/titlebar.h \
    widgets/userpopupwidget.h \
    appdatapath.h

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
    forms/userpopup.ui
