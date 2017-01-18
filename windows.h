#ifndef WINDOWS_H
#define WINDOWS_H

#include "widgets/mainwindow.h"

#include <QMutex>

using namespace chatterino::widgets;

namespace chatterino {

class Windows
{
public:
    static void layoutVisibleChatWidgets(Channel *channel = NULL);
    static void repaintVisibleChatWidgets(Channel *channel = NULL);

    static MainWindow &
    getMainWindow()
    {
        windowMutex.lock();
        if (mainWindow == NULL) {
            mainWindow = new MainWindow();
        }
        windowMutex.unlock();

        return *mainWindow;
    }

private:
    Windows()
    {
    }

    static QMutex windowMutex;

    static MainWindow *mainWindow;
};
}

#endif  // WINDOWS_H
