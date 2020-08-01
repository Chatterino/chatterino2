#pragma once

#include "widgets/dialogs/switcher/AbstractSwitcherItem.hpp"

namespace chatterino {

class QuickSwitcherModel : public QAbstractListModel
{
public:
    QuickSwitcherModel(QWidget *parent = nullptr);
    ~QuickSwitcherModel();

    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    void addItem(AbstractSwitcherItem *item);
    void clear();

private:
    QVector<AbstractSwitcherItem *> items_;
};
}  // namespace chatterino
