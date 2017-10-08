#pragma once

#include <QDebug>
#include <QElapsedTimer>

#define BENCH(x)     \
    QElapsedTimer x; \
    x.start();

#define MARK(x) \
    qDebug() << __FILE__ << __LINE__ << static_cast<float>(x.nsecsElapsed()) / 100000.0 << "ms";
