#include "controllers/highlights/HighlightBlacklistModel.hpp"

#include "Application.hpp"
#include "singletons/Settings.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
HighlightBlacklistModel::HighlightBlacklistModel(QObject *parent)
    : SignalVectorModel<HighlightBlacklistUser>(2, parent)
{
}

// turn a vector item into a model row
HighlightBlacklistUser HighlightBlacklistModel::getItemFromRow(
    std::vector<QStandardItem *> &row, const HighlightBlacklistUser &original)
{
    // key, regex

    return HighlightBlacklistUser{row[0]->data(Qt::DisplayRole).toString(),
                                  row[1]->data(Qt::CheckStateRole).toBool()};
}

// turns a row in the model into a vector item
void HighlightBlacklistModel::getRowFromItem(const HighlightBlacklistUser &item,
                                             std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item.getPattern());
    setBoolItem(row[1], item.isRegex());
}

}  // namespace chatterino
