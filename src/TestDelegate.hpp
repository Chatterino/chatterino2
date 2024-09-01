#pragma once

#include <QSize>
#include <QStyledItemDelegate>

namespace chatterino {

class TestDelegate : public QStyledItemDelegate
{
public:
    explicit TestDelegate(QObject *parent);

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};

}  // namespace chatterino
