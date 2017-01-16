#include "windows.h"

QMutex Windows::m_windowMutex;

MainWindow *Windows::m_mainWindow(NULL);

void
Windows::layoutVisibleChatWidgets()
{
    m_mainWindow->layoutVisibleChatWidgets();
}

void
Windows::repaintVisibleChatWidgets()
{
    m_mainWindow->repaintVisibleChatWidgets();
}
