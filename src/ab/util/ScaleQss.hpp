#pragma once

#include <QString>

namespace ab
{
    QString scaleQss(const QString& qss, double scale, double nativeScale);

    void testScaleQss();
}  // namespace ab
