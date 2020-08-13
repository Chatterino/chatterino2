#include "widgets/dialogs/switcher/QuickSwitcherModel.hpp"

namespace chatterino {

QuickSwitcherModel::QuickSwitcherModel(QWidget *parent)
    : QAbstractListModel(parent)
{
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

    auto item = this->items_[index.row()].get();
    // See https://stackoverflow.com/a/44503822 .
    return QVariant::fromValue(static_cast<void *>(item));
}

void QuickSwitcherModel::addItem(std::unique_ptr<AbstractSwitcherItem> item)
{
    // {begin,end}InsertRows needs to be called to notify attached views
    this->beginInsertRows(QModelIndex(), this->items_.size(),
                          this->items_.size());
    this->items_.push_back(std::move(item));
    this->endInsertRows();
}

void QuickSwitcherModel::clear()
{
    if (this->items_.empty())
        return;

    // {begin,end}RemoveRows needs to be called to notify attached views
    this->beginRemoveRows(QModelIndex(), 0, this->items_.size() - 1);

    // clear
    this->items_.clear();

    this->endRemoveRows();
}

}  // namespace chatterino
