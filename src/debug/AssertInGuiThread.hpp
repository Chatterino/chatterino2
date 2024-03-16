#pragma once

#include <QCoreApplication>
#include <QThread>

#include <cassert>

namespace chatterino {

inline bool isGuiThread()
{
    return QCoreApplication::instance()->thread() == QThread::currentThread();
}

inline void assertInGuiThread()
{
#ifdef _DEBUG
    assert(isGuiThread());
#endif
}

}  // namespace chatterino
