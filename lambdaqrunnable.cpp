#include "lambdaqrunnable.h"

LambdaQRunnable::LambdaQRunnable(std::function<void ()> action)
{
    this->action = action;
}

void LambdaQRunnable::run()
{
    action();
}
