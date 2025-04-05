#pragma once

#include "debug/AssertInGuiThread.hpp"

#include <QCoreApplication>

namespace chatterino {

// Taken from
// https://stackoverflow.com/questions/21646467/how-to-execute-a-functor-or-a-lambda-in-a-given-thread-in-qt-gcd-style
// Qt 5/4 - preferred, has least allocations
static void postToThread(auto &&f, QObject *obj = QCoreApplication::instance())
{
    struct Event : public QEvent {
        using Fun = typename std::decay_t<decltype(f)>;
        Fun fun;

        Event(decltype(fun) f)
            : QEvent(QEvent::None)
            , fun(std::forward<decltype(fun)>(f))
        {
        }
        Event(const Event &) = delete;
        Event(Event &&) = delete;
        Event &operator=(const Event &) = delete;
        Event &operator=(Event &&) = delete;

        ~Event() override
        {
            fun();
        }
    };
    QCoreApplication::postEvent(obj, new Event(std::forward<decltype(f)>(f)));
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
