#include "widgets/dialogs/switcher/QuickSwitcherModel.hpp"

namespace chatterino {

QuickSwitcherModel::QuickSwitcherModel(QWidget *parent)
    : QAbstractListModel(parent)
    , items_(INITIAL_ITEMS_SIZE)
{
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

QVariant QuickSwitcherModel::data(const QModelIndex &index,
                                  int /* role */) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= this->items_.size())
        return QVariant();

    auto item = this->items_.at(index.row());
    // See https://stackoverflow.com/a/44503822 .
    return QVariant::fromValue(static_cast<void *>(item));
}

void QuickSwitcherModel::addItem(AbstractSwitcherItem *item)
{
    // {begin,end}InsertRows needs to be called to notify attached views
    this->beginInsertRows(QModelIndex(), this->items_.size(),
                          this->items_.size());
    this->items_.append(item);
    this->endInsertRows();
}

void QuickSwitcherModel::clear()
{
    // {begin,end}RemoveRows needs to be called to notify attached views
    this->beginRemoveRows(QModelIndex(), 0, this->items_.size() - 1);

    for (AbstractSwitcherItem *item : this->items_)
    {
        delete item;
    }
    this->items_.clear();

    this->endRemoveRows();
}

}  // namespace chatterino
