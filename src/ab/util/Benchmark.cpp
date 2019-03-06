#include "Benchmark.hpp"

#include <QDebug>

namespace ab
{
    BenchmarkGuard::BenchmarkGuard(const QString& _name)
        : name_(_name)
    {
        timer_.start();
    }

    BenchmarkGuard::~BenchmarkGuard()
    {
        qDebug() << this->name_ << (timer_.nsecsElapsed() / 1000000.0f) << "ms";
    }

    qreal BenchmarkGuard::getElapsedMs()
    {
        return qreal(timer_.nsecsElapsed()) / 1000000.0;
    }

}  // namespace ab
