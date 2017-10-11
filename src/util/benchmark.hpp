#pragma once

#include <QDebug>
#include <QElapsedTimer>
#include <boost/current_function.hpp>

#define BENCH(x)     \
    QElapsedTimer x; \
    x.start();

#define MARK(x)                                    \
    qDebug() << BOOST_CURRENT_FUNCTION << __LINE__ \
             << static_cast<float>(x.nsecsElapsed()) / 100000.0 << "ms";
