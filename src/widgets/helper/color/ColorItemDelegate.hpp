#pragma once

#include <QStyledItemDelegate>

namespace chatterino {

class ColorItemDelegate : public QStyledItemDelegate
{
public:
    explicit ColorItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};

}  // namespace chatterino
