#include "controllers/moderationactions/ModerationActionModel.hpp"

#include "controllers/moderationactions/ModerationAction.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
ModerationActionModel ::ModerationActionModel(QObject *parent)
    : SignalVectorModel<ModerationAction>(2, parent)
{
}

// turn a vector item into a model row
ModerationAction ModerationActionModel::getItemFromRow(
    std::vector<QStandardItem *> &row, const ModerationAction &original)
{
    return ModerationAction(
        row[Column::Command]->data(Qt::DisplayRole).toString(),
        row[Column::Icon]->data(Qt::UserRole).toString());
}

// turns a row in the model into a vector item
void ModerationActionModel::getRowFromItem(const ModerationAction &item,
                                           std::vector<QStandardItem *> &row)
{
    setStringItem(row[Column::Command], item.getAction());
    setFilePathItem(row[Column::Icon], item.iconPath());
}

}  // namespace chatterino
