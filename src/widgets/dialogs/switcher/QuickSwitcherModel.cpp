#include "widgets/dialogs/switcher/QuickSwitcherModel.hpp"

namespace chatterino {

QuickSwitcherModel::QuickSwitcherModel(QWidget *parent)
    : QAbstractListModel(parent)
{
    // TODO(leon): init items_ with reasonable size
}

QuickSwitcherModel::~QuickSwitcherModel()
{
    for (AbstractSwitcherItem *item : this->items_)
    {
        delete item;
    }
}

int QuickSwitcherModel::rowCount(const QModelIndex &parent) const
{
    return this->items_.size();
}

QVariant QuickSwitcherModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= this->items_.size())
        return QVariant();

    // TODO(leon): check if this is safe
    /*
     * Couldn't figure out a nicer way to do this (not sure if it's even possible).
     * See https://stackoverflow.com/a/44503822 .
     */
    auto item = this->items_.at(index.row());
    return QVariant::fromValue(static_cast<void *>(item));
}

void QuickSwitcherModel::addItem(AbstractSwitcherItem *item)
{
    this->beginInsertRows(QModelIndex(), this->items_.size(),
                          this->items_.size());
    this->items_.append(item);
    this->endInsertRows();
}

void QuickSwitcherModel::clear()
{
    this->beginRemoveRows(QModelIndex(), 0, this->items_.size() - 1);

    for (AbstractSwitcherItem *item : this->items_)
    {
        delete item;
    }
    this->items_.clear();

    this->endRemoveRows();
}

}  // namespace chatterino
