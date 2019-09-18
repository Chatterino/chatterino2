#include "UserHighlightModel.hpp"

#include "Application.hpp"
#include "singletons/Settings.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
UserHighlightModel::UserHighlightModel(QObject *parent)
    : SignalVectorModel<HighlightPhrase>(5, parent)
{
}

// turn vector item into model row
HighlightPhrase UserHighlightModel::getItemFromRow(
    std::vector<QStandardItem *> &row, const HighlightPhrase &original)
{
    // key, regex

    return HighlightPhrase{row[0]->data(Qt::DisplayRole).toString(),
                           row[1]->data(Qt::CheckStateRole).toBool(),
                           row[2]->data(Qt::CheckStateRole).toBool(),
                           row[3]->data(Qt::CheckStateRole).toBool(),
                           row[4]->data(Qt::CheckStateRole).toBool()};
}

// row into vector item
void UserHighlightModel::getRowFromItem(const HighlightPhrase &item,
                                        std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item.getPattern());
    setBoolItem(row[1], item.hasAlert());
    setBoolItem(row[2], item.hasSound());
    setBoolItem(row[3], item.isRegex());
    setBoolItem(row[4], item.isCaseSensitive());
}

}  // namespace chatterino
