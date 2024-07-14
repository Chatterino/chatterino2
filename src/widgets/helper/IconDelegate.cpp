#include "widgets/helper/IconDelegate.hpp"

#include <QPainter>
#include <QVariant>

namespace chatterino {

IconDelegate::IconDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void IconDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    auto data = index.data(Qt::DecorationRole);

    if (data.type() != QVariant::Pixmap)
    {
        return QStyledItemDelegate::paint(painter, option, index);
    }

    auto scaledRect = option.rect;
    scaledRect.setWidth(scaledRect.height());

    painter->drawPixmap(scaledRect, data.value<QPixmap>());
}

}  // namespace chatterino
