#pragma once

#include <QStandardItem>
#include <QUrl>

namespace chatterino {

inline auto defaultItemFlags(bool selectable)
{
    return Qt::ItemIsEnabled |
           (selectable ? Qt::ItemIsSelectable | Qt::ItemIsDragEnabled |
                             Qt::ItemIsDropEnabled
                       : Qt::ItemFlag());
}

inline void setBoolItem(QStandardItem *item, bool value,
                        bool userCheckable = true, bool selectable = true)
{
    item->setFlags(
        Qt::ItemFlags(defaultItemFlags(selectable) |
                      (userCheckable ? Qt::ItemIsUserCheckable : 0)));
    item->setCheckState(value ? Qt::Checked : Qt::Unchecked);
}

inline void setStringItem(QStandardItem *item, const QString &value,
                          bool editable = true, bool selectable = true)
{
    item->setData(value, Qt::EditRole);
    item->setFlags(Qt::ItemFlags(defaultItemFlags(selectable) |
                                 (editable ? (Qt::ItemIsEditable) : 0)));
}

inline void setFilePathItem(QStandardItem *item, const QUrl &value,
                            bool selectable = true)
{
    item->setData(value, Qt::UserRole);
    item->setData(value.fileName(), Qt::DisplayRole);
    item->setFlags(
        Qt::ItemFlags(defaultItemFlags(selectable) |
                      (selectable ? Qt::ItemIsSelectable : Qt::NoItemFlags)));
}

inline void setColorItem(QStandardItem *item, const QColor &value,
                         bool selectable = true)
{
    item->setData(value, Qt::DecorationRole);
    item->setFlags(
        Qt::ItemFlags(defaultItemFlags(selectable) |
                      (selectable ? Qt::ItemIsSelectable : Qt::NoItemFlags)));
}

inline QStandardItem *emptyItem()
{
    auto *item = new QStandardItem();
    item->setFlags(Qt::ItemFlags());
    return item;
}

}  // namespace chatterino
