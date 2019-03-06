#pragma once

#include <QPointF>
#include <cmath>

namespace chatterino
{
    inline qreal distance(const QPointF& p1, const QPointF& p2)
    {
        auto diff = p1 - p2;

        return std::sqrt(diff.x() * diff.x() + diff.y() * diff.y());
    }
}  // namespace chatterino
