#include "Benchmark.hpp"

namespace AB_NAMESPACE {

BenchmarkGuard::BenchmarkGuard(const QString &_name)
    : name_(_name)
{
    timer_.start();
}

BenchmarkGuard::~BenchmarkGuard()
{
    log("{} {} ms", this->name_, float(timer_.nsecsElapsed()) / 1000000.0f);
}

qreal BenchmarkGuard::getElapsedMs()
{
    return qreal(timer_.nsecsElapsed()) / 1000000.0;
}

}  // namespace AB_NAMESPACE
