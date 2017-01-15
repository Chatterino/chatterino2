#include "windows.h"

QMutex Windows::m_windowMutex;

MainWindow *Windows::m_mainWindow(NULL);

void
Windows::invalidateEmotes()
{
}
