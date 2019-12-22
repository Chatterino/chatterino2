#pragma once

#include <QPointF>

#include <cmath>

namespace chatterino {

inline float distanceBetweenPoints(const QPointF &p1, const QPointF &p2)
{
    QPointF tmp = p1 - p2;

    float distance = 0.f;
    distance += tmp.x() * tmp.x();
    distance += tmp.y() * tmp.y();

    return sqrt(distance);
}

}  // namespace chatterino
