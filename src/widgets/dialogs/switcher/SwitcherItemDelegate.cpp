#include "widgets/dialogs/switcher/SwitcherItemDelegate.hpp"

#include "widgets/dialogs/switcher/AbstractSwitcherItem.hpp"

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
    auto *item = AbstractSwitcherItem::fromVariant(index.data());

    if (item)
    {
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
    auto *item = AbstractSwitcherItem::fromVariant(index.data());

    if (item)
    {
        return item->sizeHint(option.rect);
    }

    return QStyledItemDelegate::sizeHint(option, index);
}

}  // namespace chatterino
