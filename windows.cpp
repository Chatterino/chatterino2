#include "windows.h"

QMutex Windows::windowMutex;

MainWindow *Windows::mainWindow(NULL);

void
Windows::layoutVisibleChatWidgets(Channel *channel)
{
    Windows::mainWindow->layoutVisibleChatWidgets(channel);
}

void
Windows::repaintVisibleChatWidgets(Channel *channel)
{
    Windows::mainWindow->repaintVisibleChatWidgets(channel);
}
