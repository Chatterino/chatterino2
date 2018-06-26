#pragma once

#include <QCoreApplication>
#include <QThread>
#include <cassert>

namespace chatterino {

static void assertInGuiThread()
{
#ifdef _DEBUG
    assert(QCoreApplication::instance()->thread() == QThread::currentThread());
#endif
}

}  // namespace chatterino
