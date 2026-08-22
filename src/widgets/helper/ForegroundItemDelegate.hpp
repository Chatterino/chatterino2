// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QStyledItemDelegate>

namespace chatterino {

/// An item delegate that keeps an item's Qt::ForegroundRole color visible
/// while the item is selected, by using it as the selection color.
///
/// By default a selected item is painted in the usual selection colors, which
/// hides any color an item uses to mark itself - exactly when the user has
/// that item selected.
class ForegroundItemDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};

}  // namespace chatterino
