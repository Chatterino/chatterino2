#-------------------------------------------------
#
# Project created by QtCreator 2016-12-28T18:23:35
#
#-------------------------------------------------

QT       += core gui network
CONFIG += communi
COMMUNI += core model util

#win32 {
#    LIBS += -L"C:/OpenSSL-Win32/lib" -llibssl.lib
#    INCLUDEPATH += C:/OpenSSL-Win32/include
#} else {
#    LIBS += -lcrypto -lssl
#}

CONFIG += c++14

include(lib/libcommuni/src/src.pri)

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = chatterino
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
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
    settings/settings.cpp \
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
    windows.cpp

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
    settings/setting.h \
    settings/settings.h \
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
    settings/boolsetting.h \
    settings/stringsetting.h \
    settings/intsetting.h \
    settings/floatsetting.h \
    widgets/resizingtextedit.h

PRECOMPILED_HEADER =

RESOURCES += \
    resources.qrc

DISTFILES +=


# Include boost
win32 {
        INCLUDEPATH += C:\local\boost\
}
