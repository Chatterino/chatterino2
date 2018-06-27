#pragma once

#include <QCoreApplication>

#include <QRunnable>
#include <QThreadPool>

#include <functional>

#define async_exec(a) QThreadPool::globalInstance()->start(new LambdaRunnable(a));

namespace chatterino {

class LambdaRunnable : public QRunnable
{
public:
    LambdaRunnable(std::function<void()> action)
    {
        this->action = action;
    }

    void run()
    {
        this->action();
    }

private:
    std::function<void()> action;
};

// Taken from
// https://stackoverflow.com/questions/21646467/how-to-execute-a-functor-or-a-lambda-in-a-given-thread-in-qt-gcd-style
// Qt 5/4 - preferred, has least allocations
template <typename F>
static void postToThread(F &&fun, QObject *obj = qApp)
{
    struct Event : public QEvent {
        using Fun = typename std::decay<F>::type;
        Fun fun;
        Event(Fun &&fun)
            : QEvent(QEvent::None)
            , fun(std::move(fun))
        {
        }
        Event(const Fun &fun)
            : QEvent(QEvent::None)
            , fun(fun)
        {
        }
        ~Event() override
        {
            fun();
        }
    };
    QCoreApplication::postEvent(obj, new Event(std::forward<F>(fun)));
}

}  // namespace chatterino
