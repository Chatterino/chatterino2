#pragma once

#include <QStandardItem>

namespace chatterino {
namespace util {

static QStandardItem *boolItem(bool value, bool userCheckable = true, bool selectable = true)
{
    auto *item = new QStandardItem();
    item->setFlags((Qt::ItemFlags)(Qt::ItemIsEnabled | (selectable ? Qt::ItemIsSelectable : 0) |
                                   (userCheckable ? Qt::ItemIsUserCheckable : 0)));
    item->setCheckState(value ? Qt::Checked : Qt::Unchecked);
    return item;
}

static QStandardItem *stringItem(const QString &value, bool editable = true, bool selectable = true)
{
    auto *item = new QStandardItem(value);
    item->setFlags((Qt::ItemFlags)(Qt::ItemIsEnabled | (selectable ? Qt::ItemIsSelectable : 0) |
                                   (editable ? (Qt::ItemIsEditable) : 0)));
    return item;
}

static QStandardItem *emptyItem()
{
    auto *item = new QStandardItem();
    item->setFlags((Qt::ItemFlags)0);
    return item;
}

}  // namespace util
}  // namespace chatterino
