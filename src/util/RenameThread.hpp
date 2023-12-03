#pragma once

#include <QtGlobal>

#ifdef Q_OS_LINUX
#    include <pthread.h>
#endif

namespace chatterino {

template <typename T>
void renameThread(T &&thread, const char *threadName)
{
#ifdef Q_OS_LINUX
    pthread_setname_np(thread.native_handle(), threadName);
#endif
}

}  // namespace chatterino
