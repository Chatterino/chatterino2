#pragma once

#include "debug/AssertInGuiThread.hpp"

#include <QCoreApplication>
#include <QMetaObject>

namespace chatterino {

static void postToThread(auto &&f, QObject *obj = QCoreApplication::instance())
{
    QMetaObject::invokeMethod(obj, std::forward<decltype(f)>(f));
}

static void runInGuiThread(auto &&fun)
{
    if (isGuiThread())
    {
        fun();
    }
    else
    {
        postToThread(std::forward<decltype(fun)>(fun));
    }
}

inline void postToGuiThread(auto &&fun)
{
    assert(!isGuiThread() &&
           "postToGuiThread must be called from a non-GUI thread");

    postToThread(std::forward<decltype(fun)>(fun));
}

}  // namespace chatterino
