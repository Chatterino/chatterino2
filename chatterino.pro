#-------------------------------------------------
#
# Project created by QtCreator 2016-12-28T18:23:35
#
#-------------------------------------------------

QT       += core gui network
CONFIG += communi
COMMUNI += core model util

win32:LIBS += -LC:/OpenSSL-Win32/lib/openssl.lib
INCLUDEPATH += C:/OpenSSL-Win32/include

CONFIG += c++11

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
    mainwindow.cpp \
    chatwidget.cpp \
    notebook.cpp \
    notebooktab.cpp \
    notebookpage.cpp \
    notebookbutton.cpp \
    colorscheme.cpp \
    chatwidgetheader.cpp \
    chatwidgetinput.cpp \
    chatwidgetview.cpp \
    notebookpagedroppreview.cpp \
    channel.cpp \
    dialog.cpp \
    settingsdialog.cpp \
    settingsdialogtab.cpp \
    scrollbar.cpp \
    scrollbarhighlight.cpp \
    ircmanager.cpp \
    lambdaqrunnable.cpp \
    account.cpp \
    emotes.cpp \
    lazyloadedimage.cpp \
    concurrentmap.cpp \
    message.cpp \
    word.cpp \
    link.cpp \
    fonts.cpp \
    appsettings.cpp \
    emojis.cpp \
    wordpart.cpp \
    resources.cpp \
    windows.cpp \
    chatwidgetheaderbutton.cpp \
    chatwidgetheaderbuttonlabel.cpp \
    channels.cpp \
    textinputform.cpp

HEADERS  += mainwindow.h \
    chatwidget.h \
    notebook.h \
    notebooktab.h \
    notebookpage.h \
    notebookbutton.h \
    colorscheme.h \
    chatwidgetheader.h \
    chatwidgetinput.h \
    chatwidgetview.h \
    notebookpagedroppreview.h \
    channel.h \
    dialog.h \
    settingsdialog.h \
    settingsdialogtab.h \
    scrollbar.h \
    scrollbarhighlight.h \
    ircmanager.h \
    lambdaqrunnable.h \
    asyncexec.h \
    account.h \
    emotes.h \
    lazyloadedimage.h \
    twitchemotevalue.h \
    concurrentmap.h \
    message.h \
    word.h \
    link.h \
    fonts.h \
    appsettings.h \
    emojis.h \
    wordpart.h \
    common.h \
    resources.h \
    windows.h \
    chatwidgetheaderbutton.h \
    chatwidgetheaderbuttonlabel.h \
    channels.h \
    textinputform.h

PRECOMPILED_HEADER =

FORMS    += \
    dialog.ui

RESOURCES += \
    resources.qrc

DISTFILES +=
