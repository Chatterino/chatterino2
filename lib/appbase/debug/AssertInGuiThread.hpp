#pragma once

#include <QCoreApplication>
#include <QThread>
#include <cassert>

namespace AB_NAMESPACE {

static void assertInGuiThread()
{
#ifdef _DEBUG
    assert(QCoreApplication::instance()->thread() == QThread::currentThread());
#endif
}

}  // namespace AB_NAMESPACE
