// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/helper/ForegroundItemDelegate.hpp"

#include <QApplication>
#include <QBrush>
#include <QPainter>
#include <QStyle>

namespace chatterino {

void ForegroundItemDelegate::paint(QPainter *painter,
                                   const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    auto foreground = index.data(Qt::ForegroundRole);
    if (!option.state.testFlag(QStyle::State_Selected) ||
        !foreground.canConvert<QBrush>())
    {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QStyleOptionViewItem opt(option);
    this->initStyleOption(&opt, index);

    auto brush = qvariant_cast<QBrush>(foreground);
    if (opt.state.testFlag(QStyle::State_MouseOver))
    {
        // Keep a hover cue, since the view's own hover color is one of the
        // things being painted over here
        brush = QBrush(brush.color().lighter(115));
    }

    // Views can set their selection and hover backgrounds through a
    // stylesheet, which wins over anything we put in the palette. So fill the
    // row ourselves and take both states away from the style.
    painter->fillRect(opt.rect, brush);
    opt.backgroundBrush = brush;
    opt.state &= ~(QStyle::State_Selected | QStyle::State_MouseOver);

    // The row is now the marker color in every theme, so the text has to be
    // readable on that rather than on the usual selection color.
    opt.palette.setBrush(QPalette::Text, Qt::white);

    const auto *style =
        opt.widget ? opt.widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
}

}  // namespace chatterino
