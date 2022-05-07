#pragma once

#include <QStyledItemDelegate>

namespace chatterino {

class RegExpItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    RegExpItemDelegate(QObject *parent, QString pattern);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

private:
    QString pattern_;
};

}  // namespace chatterino
