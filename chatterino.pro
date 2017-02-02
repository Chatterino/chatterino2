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

#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += main.cpp\
    account.cpp \
    channel.cpp \
    channels.cpp \
    colorscheme.cpp \
    emojis.cpp \
    emotes.cpp \
    fonts.cpp \
    ircmanager.cpp \
    messages/lazyloadedimage.cpp \
    messages/link.cpp \
    messages/message.cpp \
    messages/word.cpp \
    messages/wordpart.cpp \
    resources.cpp \
    settings.cpp \
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
    windows.cpp \
    logging.cpp \
    messages/messageref.cpp

HEADERS  += account.h \
    asyncexec.h \
    channel.h \
    channels.h \
    colorscheme.h \
    common.h \
    concurrentmap.h \
    emojis.h \
    emotes.h \
    fonts.h \
    ircmanager.h \
    messages/lazyloadedimage.h \
    messages/link.h \
    messages/message.h \
    messages/word.h \
    messages/wordpart.h \
    resources.h \
    setting.h \
    settings.h \
    twitchemotevalue.h \
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
    windows.h \
    widgets/resizingtextedit.h \
    settingssnapshot.h \
    logging.h \
    messages/limitedqueue.h \
    messages/limitedqueuesnapshot.h \
    messages/messageref.h

PRECOMPILED_HEADER =

RESOURCES += \
    resources.qrc

DISTFILES +=

# Include boost
win32 {
        INCLUDEPATH += C:\local\boost\
}

macx {
    INCLUDEPATH += /usr/local/include
}
