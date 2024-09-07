#pragma once

#include <QString>
#include <QtGlobal>

#ifdef Q_OS_LINUX
#    include <pthread.h>
#endif

namespace chatterino {

#ifdef Q_OS_WIN
namespace windows::detail {
    void renameThread(void *hThread, const QString &name);
}  // namespace windows::detail
#endif

template <typename T>
void renameThread(T &thread, const QString &threadName)
{
#ifdef Q_OS_LINUX
    pthread_setname_np(thread.native_handle(), threadName.toLocal8Bit());
#elif defined(Q_OS_WIN)
    windows::detail::renameThread(thread.native_handle(), threadName);
#endif
}

}  // namespace chatterino
