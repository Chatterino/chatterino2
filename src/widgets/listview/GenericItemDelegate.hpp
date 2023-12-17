#pragma once

#include <QStyledItemDelegate>

namespace chatterino {

class SwitcherItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    SwitcherItemDelegate(QObject *parent = nullptr);
    ~SwitcherItemDelegate() override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
};

}  // namespace chatterino
