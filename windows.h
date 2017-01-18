#ifndef WINDOWS_H
#define WINDOWS_H

#include "widgets/mainwindow.h"

#include <QMutex>

namespace chatterino {

class Windows
{
public:
    static void layoutVisibleChatWidgets(Channel *channel = NULL);
    static void repaintVisibleChatWidgets(Channel *channel = NULL);

    static widgets::MainWindow &
    getMainWindow()
    {
        windowMutex.lock();
        if (mainWindow == NULL) {
            mainWindow = new widgets::MainWindow();
        }
        windowMutex.unlock();

        return *mainWindow;
    }

private:
    Windows()
    {
    }

    static QMutex windowMutex;

    static widgets::MainWindow *mainWindow;
};
}

#endif  // WINDOWS_H
