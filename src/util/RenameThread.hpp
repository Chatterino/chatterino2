#pragma once

#include <QString>
#include <QtGlobal>

#ifdef Q_OS_LINUX
#    include <pthread.h>
#endif

namespace chatterino {

template <typename T>
void renameThread(T &&thread, const QString &threadName)
{
#ifdef Q_OS_LINUX
    pthread_setname_np(thread.native_handle(), threadName.toLocal8Bit());
#endif
}

}  // namespace chatterino
