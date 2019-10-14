#pragma once

#include <QStandardItem>

namespace chatterino {

static void setBoolItem(QStandardItem *item, bool value,
                        bool userCheckable = true, bool selectable = true)
{
    item->setFlags(Qt::ItemFlags(
        Qt::ItemIsEnabled | (selectable ? Qt::ItemIsSelectable : 0) |
        (userCheckable ? Qt::ItemIsUserCheckable : 0)));
    item->setCheckState(value ? Qt::Checked : Qt::Unchecked);
}

static void setStringItem(QStandardItem *item, const QString &value,
                          bool editable = true, bool selectable = true)
{
    item->setData(value, Qt::EditRole);
    item->setFlags(Qt::ItemFlags(Qt::ItemIsEnabled |
                                 (selectable ? Qt::ItemIsSelectable : 0) |
                                 (editable ? (Qt::ItemIsEditable) : 0)));
}

static void setFilePathItem(QStandardItem *item, const QUrl &value)
{
    item->setData(value, Qt::UserRole);
    item->setData(value.fileName(), Qt::DisplayRole);
    item->setFlags(Qt::ItemFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable));
}

static void setColorItem(QStandardItem *item, const QColor &value)
{
    item->setData(value, Qt::DecorationRole);
    item->setFlags(Qt::ItemFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable));
}

static QStandardItem *emptyItem()
{
    auto *item = new QStandardItem();
    item->setFlags(Qt::ItemFlags(0));
    return item;
}

}  // namespace chatterino
