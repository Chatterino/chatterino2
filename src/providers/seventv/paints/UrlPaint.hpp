#pragma once

#include "Paint.hpp"
#include "messages/Image.hpp"

namespace chatterino {

class UrlPaint : public Paint
{
public:
    UrlPaint(const QString name, const ImagePtr image,
             const std::vector<PaintDropShadow>);

    QBrush asBrush(const QColor userColor,
                   const QRectF drawingRect) const override;
    std::vector<PaintDropShadow> getDropShadows() const override;
    bool animated() const override;

private:
    const QString name_;
    const ImagePtr image_;

    const std::vector<PaintDropShadow> dropShadows_;
};

}  // namespace chatterino
