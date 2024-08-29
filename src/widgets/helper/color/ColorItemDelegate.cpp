#include "widgets/helper/color/ColorItemDelegate.hpp"

#include "widgets/helper/color/Checkerboard.hpp"

namespace chatterino {

ColorItemDelegate::ColorItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void ColorItemDelegate::paint(QPainter *painter,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    auto data = index.data(Qt::DecorationRole);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (data.typeId() != QMetaType::QColor)
#else
    if (data.type() != QVariant::Color)
#endif
    {
        return QStyledItemDelegate::paint(painter, option, index);
    }
    auto color = data.value<QColor>();

    painter->save();
    if (color.alpha() != 255)
    {
        drawCheckerboard(*painter, option.rect,
                         std::min(option.rect.height() / 2, 10));
    }
    painter->setBrush(color);
    painter->drawRect(option.rect);
    painter->restore();
}

}  // namespace chatterino
