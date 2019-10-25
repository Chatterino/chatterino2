#include "UserHighlightModel.hpp"

#include "Application.hpp"
#include "controllers/highlights/HighlightModel.hpp"
#include "singletons/Settings.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
UserHighlightModel::UserHighlightModel(QObject *parent)
    : SignalVectorModel<HighlightPhrase>(7, parent)
{
}

// turn vector item into model row
HighlightPhrase UserHighlightModel::getItemFromRow(
    std::vector<QStandardItem *> &row, const HighlightPhrase &original)
{
    using Column = HighlightModel::Column;

    return HighlightPhrase{
        row[Column::Pattern]->data(Qt::DisplayRole).toString(),
        row[Column::FlashTaskbar]->data(Qt::CheckStateRole).toBool(),
        row[Column::PlaySound]->data(Qt::CheckStateRole).toBool(),
        row[Column::UseRegex]->data(Qt::CheckStateRole).toBool(),
        row[Column::CaseSensitive]->data(Qt::CheckStateRole).toBool(),
        row[Column::SoundPath]->data(Qt::UserRole).toString(),
        std::make_shared<QColor>(
            row[Column::Color]->data(Qt::DecorationRole).value<QColor>())};
}

// row into vector item
void UserHighlightModel::getRowFromItem(const HighlightPhrase &item,
                                        std::vector<QStandardItem *> &row)
{
    using Column = HighlightModel::Column;

    setStringItem(row[Column::Pattern], item.getPattern());
    setBoolItem(row[Column::FlashTaskbar], item.hasAlert());
    setBoolItem(row[Column::PlaySound], item.hasSound());
    setBoolItem(row[Column::UseRegex], item.isRegex());
    setBoolItem(row[Column::CaseSensitive], item.isCaseSensitive());
    setFilePathItem(row[Column::SoundPath], item.getSoundUrl());
    setColorItem(row[Column::Color], *item.getColor());
}

}  // namespace chatterino
