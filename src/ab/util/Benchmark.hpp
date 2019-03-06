#pragma once

#include <QElapsedTimer>
#include <QString>
#include <boost/noncopyable.hpp>

namespace ab
{
    class BenchmarkGuard
    {
    public:
        BenchmarkGuard(const QString& _name);
        ~BenchmarkGuard();
        qreal getElapsedMs();

    private:
        QElapsedTimer timer_;
        QString name_;
    };

}  // namespace ab
