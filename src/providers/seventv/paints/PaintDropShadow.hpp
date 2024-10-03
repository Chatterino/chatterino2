#pragma once

#include <QColor>

class QPixmapDropShadowFilter;

namespace chatterino {

class PaintDropShadow
{
public:
    PaintDropShadow(float xOffset, float yOffset, float radius, QColor color);

    bool isValid() const;
    PaintDropShadow scaled(float scale) const;
    void apply(QPixmapDropShadowFilter &effect) const;

private:
    const float xOffset_;
    const float yOffset_;
    const float radius_;
    const QColor color_;
};

}  // namespace chatterino
