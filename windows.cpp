#include "windows.h"

namespace chatterino {

QMutex Windows::windowMutex;

widgets::MainWindow *Windows::mainWindow(nullptr);

void
Windows::layoutVisibleChatWidgets(Channel *channel)
{
    if (Windows::mainWindow != nullptr) {
        Windows::mainWindow->layoutVisibleChatWidgets(channel);
    }
}

void
Windows::repaintVisibleChatWidgets(Channel *channel)
{
    if (Windows::mainWindow != nullptr) {
        Windows::mainWindow->repaintVisibleChatWidgets(channel);
    }
}
}
