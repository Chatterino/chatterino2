#include "widgets/listview/GenericItemDelegate.hpp"

#include "widgets/listview/GenericListItem.hpp"

namespace chatterino {

SwitcherItemDelegate::SwitcherItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

SwitcherItemDelegate::~SwitcherItemDelegate()
{
}

void SwitcherItemDelegate::paint(QPainter *painter,
                                 const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    auto *item = GenericListItem::fromVariant(index.data());

    if (item)
    {
        if (option.state & QStyle::State_Selected)
        {
            painter->fillRect(option.rect, option.palette.highlight());
        }

        item->paint(painter, option.rect);
    }
    else
    {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

QSize SwitcherItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const
{
    auto *item = GenericListItem::fromVariant(index.data());

    if (item)
    {
        return item->sizeHint(option.rect);
    }

    return QStyledItemDelegate::sizeHint(option, index);
}

}  // namespace chatterino
