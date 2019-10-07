#pragma once

#include <QCoreApplication>
#include <QThread>
#include <cassert>

namespace chatterino {

static bool isGuiThread()
{
    return QCoreApplication::instance()->thread() == QThread::currentThread();
}

static void assertInGuiThread()
{
#ifdef _DEBUG
    assert(isGuiThread());
#endif
}

}  // namespace chatterino
