#include "ModerationActionModel.hpp"

#include "util/StandardItemHelper.hpp"

namespace chatterino
{
    // commandmodel
    ModerationActionModel ::ModerationActionModel(QObject* parent)
        : SignalVectorModel<ModerationAction>(1, parent)
    {
    }

    // turn a vector item into a model row
    ModerationAction ModerationActionModel::getItemFromRow(
        std::vector<QStandardItem*>& row, const ModerationAction& original)
    {
        return ModerationAction(row[0]->data(Qt::DisplayRole).toString());
    }

    // turns a row in the model into a vector item
    void ModerationActionModel::getRowFromItem(
        const ModerationAction& item, std::vector<QStandardItem*>& row)
    {
        setStringItem(row[0], item.getAction());
    }

}  // namespace chatterino
