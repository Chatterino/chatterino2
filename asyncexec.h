#ifndef ASYNCEXEC_H
#define ASYNCEXEC_H

#include "QRunnable"
#include "QThreadPool"
#include "lambdaqrunnable.h"
#include "qcoreapplication.h"

#define async_exec(a) \
    QThreadPool::globalInstance()->start(new LambdaQRunnable(a));

#endif  // ASYNCEXEC_H
