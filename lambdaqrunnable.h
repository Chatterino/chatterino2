#ifndef LAMBDAQRUNNABLE_H
#define LAMBDAQRUNNABLE_H

#include "QRunnable"
#include "functional"

class LambdaQRunnable : public QRunnable
{
public:
    LambdaQRunnable(std::function<bool ()> action);

    void run();

private:
    std::function<bool ()> action;
};

#endif // LAMBDAQRUNNABLE_H
