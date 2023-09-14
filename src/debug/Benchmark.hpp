#pragma once

#include <QElapsedTimer>
#include <QString>

namespace chatterino {

class BenchmarkGuard
{
public:
    BenchmarkGuard(const QString &_name);
    ~BenchmarkGuard();

    BenchmarkGuard(const BenchmarkGuard &) = delete;
    BenchmarkGuard &operator=(const BenchmarkGuard &) = delete;

    BenchmarkGuard(BenchmarkGuard &&) = delete;
    BenchmarkGuard &operator=(BenchmarkGuard &&) = delete;

    qreal getElapsedMs();

private:
    QElapsedTimer timer_;
    QString name_;
};

}  // namespace chatterino
