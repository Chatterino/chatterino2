#pragma once

#include "messages/Image.hpp"
#include "providers/seventv/paints/Paint.hpp"

namespace chatterino {

class UrlPaint : public Paint
{
public:
    UrlPaint(QString name, QString id, ImagePtr image,
             std::vector<PaintDropShadow> dropShadows);

    QBrush asBrush(QColor userColor, QRectF drawingRect) const override;
    const std::vector<PaintDropShadow> &getDropShadows() const override;
    bool animated() const override;

private:
    const QString name_;
    const ImagePtr image_;

    const std::vector<PaintDropShadow> dropShadows_;
};

}  // namespace chatterino
