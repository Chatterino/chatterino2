#include "UserHighlightModel.hpp"

#include "Application.hpp"
#include "singletons/Settings.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
UserHighlightModel::UserHighlightModel(QObject *parent)
    : SignalVectorModel<UserHighlight>(4, parent)
{
}

// turn vector item into model row
UserHighlight UserHighlightModel::getItemFromRow(std::vector<QStandardItem *> &row,
                                                 const UserHighlight &original)
{
    // key, regex

    return UserHighlight{
        row[0]->data(Qt::DisplayRole).toString(), row[1]->data(Qt::CheckStateRole).toBool(),
        row[2]->data(Qt::CheckStateRole).toBool(), row[3]->data(Qt::CheckStateRole).toBool()};
}

// row into vector item
void UserHighlightModel::getRowFromItem(const UserHighlight &item,
                                        std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item.getPattern());
    setBoolItem(row[1], item.getAlert());
    setBoolItem(row[2], item.getSound());
    setBoolItem(row[3], item.isRegex());
}

}  // namespace chatterino
