#ifndef WINDOWS_H
#define WINDOWS_H

#include "mainwindow.h"

#include <QMutex>

class Windows
{
public:
    static void layoutVisibleChatWidgets();
    static void repaintVisibleChatWidgets();

    static MainWindow &
    mainWindow()
    {
        m_windowMutex.lock();
        if (m_mainWindow == NULL) {
            m_mainWindow = new MainWindow();
        }
        m_windowMutex.unlock();

        return *m_mainWindow;
    }

private:
    Windows()
    {
    }

    static QMutex m_windowMutex;

    static MainWindow *m_mainWindow;
};

#endif  // WINDOWS_H
