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
    static void updateAll();

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

    static void load();
    static void save();

private:
    Windows()
    {
    }

    static QMutex windowMutex;

    static widgets::MainWindow *mainWindow;
};

}  // namespace chatterino

#endif  // WINDOWS_H
