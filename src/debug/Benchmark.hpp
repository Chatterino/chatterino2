#pragma once

#include <boost/noncopyable.hpp>
#include <QElapsedTimer>
#include <QString>

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
