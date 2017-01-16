#include "windows.h"

QMutex Windows::m_windowMutex;

MainWindow *Windows::m_mainWindow(NULL);

void
Windows::layoutVisibleChatWidgets(Channel *channel)
{
    m_mainWindow->layoutVisibleChatWidgets(channel);
}

void
Windows::repaintVisibleChatWidgets(Channel *channel)
{
    m_mainWindow->repaintVisibleChatWidgets(channel);
}
