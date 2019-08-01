#pragma once

#include <QCoreApplication>
#include <QThread>
#include <cassert>

namespace AB_NAMESPACE {

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

}  // namespace AB_NAMESPACE
