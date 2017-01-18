#ifndef ASYNCEXEC_H
#define ASYNCEXEC_H

#include "lambdaqrunnable.h"
#include "qcoreapplication.h"

#include <QRunnable>
#include <QThreadPool>

#define async_exec(a) \
    QThreadPool::globalInstance()->start(new LambdaQRunnable(a));

#endif  // ASYNCEXEC_H
