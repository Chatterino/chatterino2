#pragma once

#include <QRegularExpression>
#include <QStyledItemDelegate>

namespace chatterino {

class RegExpItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    RegExpItemDelegate(QObject *parent, QRegularExpression regexp);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

private:
    const QRegularExpression regexp_;
};

}  // namespace chatterino
