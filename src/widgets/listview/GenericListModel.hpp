#include "widgets/listview/GenericListItem.hpp"

namespace chatterino {

class GenericListModel : public QAbstractListModel
{
public:
    GenericListModel(QWidget *parent = nullptr);

    /**
     * @brief   Reimplements QAbstractItemModel::rowCount.
     *
     * @return  number of items currrently present in this model
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    /**
     * @brief   Reimplements QAbstractItemModel::data. Currently, the role parameter
     *          is not used and an GenericListItem * is always returned.
     *
     * @param   index   index of item to fetch data from
     * @param   role    (not used)
     *
     * @return  GenericListItem * (wrapped as QVariant) at index
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
    void addItem(std::unique_ptr<GenericListItem> item);

    /**
     * @brief   Clears this QuickSwitcherModel of all items. This will delete all
     *          GenericListItems added after the last invokation of
     *          QuickSwitcherModel::clear (and invalidate their pointers).
     */
    void clear();

private:
    std::vector<std::unique_ptr<GenericListItem>> items_;
};
}  // namespace chatterino
