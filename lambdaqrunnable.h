#ifndef LAMBDAQRUNNABLE_H
#define LAMBDAQRUNNABLE_H

#include "QRunnable"
#include "functional"

class LambdaQRunnable : public QRunnable
{
public:
    LambdaQRunnable(std::function<void ()> action);

    void run();

private:
    std::function<void ()> action;
};

#endif // LAMBDAQRUNNABLE_H
