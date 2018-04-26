#pragma once

#include <QCoreApplication>
#include <QThread>
#include <cassert>

namespace chatterino {
namespace util {

void assertInGuiThread()
{
#ifdef _DEBUG
    assert(QCoreApplication::instance()->thread() == QThread::currentThread());
#endif
}

}  // namespace util
}  // namespace chatterino
