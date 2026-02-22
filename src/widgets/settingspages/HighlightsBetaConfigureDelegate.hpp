#pragma once

#include <QStyledItemDelegate>

namespace chatterino {

class HighlightsBetaConfigureDelegate : public QStyledItemDelegate
{
public:
    explicit HighlightsBetaConfigureDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};

}  // namespace chatterino
