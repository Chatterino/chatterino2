// SPDX-FileCopyrightText: 2020 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/listview/GenericItemDelegate.hpp"

#include "widgets/listview/GenericListItem.hpp"

#include <QAbstractItemView>

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
        QRect rect = option.rect;
        if (auto *view = qobject_cast<const QAbstractItemView *>(this->parent()))
        {
            const int viewportWidth = view->viewport()->width();
            if (viewportWidth > 0)
            {
                rect.setWidth(viewportWidth);
            }
        }

        return item->sizeHint(rect);
    }

    return QStyledItemDelegate::sizeHint(option, index);
}

}  // namespace chatterino
