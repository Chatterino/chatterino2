#pragma once

#include <QPointF>

#include <cmath>

namespace chatterino {

inline qreal distanceBetweenPoints(const QPointF &p1, const QPointF &p2)
{
    QPointF tmp = p1 - p2;

    qreal distance = tmp.x() * tmp.x() + tmp.y() * tmp.y();

    return std::sqrt(distance);
}

}  // namespace chatterino
