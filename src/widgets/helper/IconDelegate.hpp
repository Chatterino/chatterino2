#pragma once

#include <QStyledItemDelegate>

namespace chatterino {

/**
 * IconDelegate draws the decoration role pixmap scaled down to a square icon
 */
class IconDelegate : public QStyledItemDelegate
{
public:
    explicit IconDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};

}  // namespace chatterino
