#include "lambdaqrunnable.h"

LambdaQRunnable::LambdaQRunnable(std::function<bool ()> action)
{
    this->action = action;
}

void LambdaQRunnable::run()
{
    action();
}
