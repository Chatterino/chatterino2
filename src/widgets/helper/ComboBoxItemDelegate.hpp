#pragma once

#include <QStyledItemDelegate>

namespace chatterino {

// stolen from https://wiki.qt.io/Combo_Boxes_in_Item_Views

class ComboBoxItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    ComboBoxItemDelegate(QObject *parent = 0);
    ~ComboBoxItemDelegate();

    virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const;
    virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;
    virtual void setModelData(QWidget *editor, QAbstractItemModel *model,
                              const QModelIndex &index) const;
};

}  // namespace chatterino
