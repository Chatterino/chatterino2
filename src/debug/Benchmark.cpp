#include "Benchmark.hpp"
#include "qlogging.hpp"

namespace chatterino {

BenchmarkGuard::BenchmarkGuard(const QString &_name)
    : name_(_name)
{
    timer_.start();
}

BenchmarkGuard::~BenchmarkGuard()
{
    qCDebug(chatterinoBenchmark) << this->name_ << float(timer_.nsecsElapsed()) / 1000000.0f
             << "ms";
}

qreal BenchmarkGuard::getElapsedMs()
{
    return qreal(timer_.nsecsElapsed()) / 1000000.0;
}

}  // namespace chatterino
