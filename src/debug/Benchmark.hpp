#pragma once

#include <QDebug>
#include <QElapsedTimer>
#include <boost/noncopyable.hpp>

namespace chatterino {

class BenchmarkGuard : boost::noncopyable
{
public:
    BenchmarkGuard(const QString &_name);
    ~BenchmarkGuard();
    qreal getElapsedMs();

private:
    QElapsedTimer timer_;
    QString name_;
};

}  // namespace chatterino
