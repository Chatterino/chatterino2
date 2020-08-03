#pragma once

#include "widgets/dialogs/switcher/AbstractSwitcherItem.hpp"

namespace chatterino {

class QuickSwitcherModel : public QAbstractListModel
{
public:
    QuickSwitcherModel(QWidget *parent = nullptr);
    ~QuickSwitcherModel();

    /**
     * @brief   Reimplements QAbstractItemModel::rowCount.
     *
     * @return  number of items currrently present in this model
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    /**
     * @brief   Reimplements QAbstractItemModel::data. Currently, the role parameter
     *          is not used and an AbstractSwitcherItem * is always returned.
     *
     * @param   index   index of item to fetch data from
     * @param   role    (not used)
     *
     * @return  AbstractSwitcherItem * (wrapped as QVariant) at index
     */
    QVariant data(const QModelIndex &index, int role) const;

    /**
     * @brief   Add an item to this QuickSwitcherModel. It will be displayed in
     *          attached views.
     *
     *          NOTE: The model will take ownership of the pointer. In particular,
     *          the same item should not be passed to multiple QuickSwitcherModels.
     *
     * @param   item    item to add to the model
     */
    void addItem(AbstractSwitcherItem *item);

    /**
     * @brief   Clears this QuickSwitcherModel of all items. This will delete all
     *          AbstractSwitcherItems added after the last invokation of
     *          QuickSwitcherModel::clear (and invalidate their pointers).
     */
    void clear();

private:
    /*
     * On my system, the default QVector capacity is 0. 20 is an attempt at preventing
     * frequent reallocations. The number is not backed by any user data but rather a
     * guess at how many switcher items are probably going to be added.
     */
    static constexpr int INITIAL_ITEMS_SIZE = 20;

    QVector<AbstractSwitcherItem *> items_;
};
}  // namespace chatterino
