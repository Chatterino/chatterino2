#ifndef ASYNCEXEC_H
#define ASYNCEXEC_H

#include "QThreadPool"
#include "QRunnable"
#include "lambdaqrunnable.h"
#include "qcoreapplication.h"

#define async_start QThreadPool::globalInstance()->start(new LambdaQRunnable(
#define async_end ));
#define async_exec(a) QThreadPool::globalInstance()->start(new LambdaQRunnable([]{ a; }));


#endif // ASYNCEXEC_H
