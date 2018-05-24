#pragma once

#include <QDebug>
#include <QElapsedTimer>
#include <boost/current_function.hpp>
#include <boost/noncopyable.hpp>

#define BENCH(x)     \
    QElapsedTimer x; \
    x.start();

#define MARK(x)                                    \
    qDebug() << BOOST_CURRENT_FUNCTION << __LINE__ \
             << static_cast<float>(x.nsecsElapsed()) / 100000.0 << "ms";

class BenchmarkGuard : boost::noncopyable {
    QElapsedTimer timer;
    QString name;

public:
    BenchmarkGuard(const QString &_name)
        :name(_name)
    {
        timer.start();
    }

    ~BenchmarkGuard() {
        qDebug() << this->name << float(timer.nsecsElapsed()) / 100000.0 << "ms";
    }

    qreal getElapsedMs() {
        return qreal(timer.nsecsElapsed()) / 100000.0;
    }
};
