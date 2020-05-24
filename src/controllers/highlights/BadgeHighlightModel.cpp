#include "BadgeHighlightModel.hpp"

#include "Application.hpp"
#include "messages/Emote.hpp"
#include "singletons/Settings.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
BadgeHighlightModel::BadgeHighlightModel(QObject *parent)
    : SignalVectorModel<HighlightBadge>(5, parent)
{
}

// turn vector item into model row
HighlightBadge BadgeHighlightModel::getItemFromRow(
    std::vector<QStandardItem *> &row, const HighlightBadge &original)
{
    using Column = BadgeHighlightModel::Column;

    // In order for old messages to update their highlight color, we need to
    // update the highlight color here.
    auto highlightColor = original.getColor();
    *highlightColor =
        row[Column::Color]->data(Qt::DecorationRole).value<QColor>();

    return HighlightBadge{
        original.badgeName(),
        original.badgeVersion(),
        row[Column::Badge]->data(Qt::DisplayRole).toString(),
        row[Column::FlashTaskbar]->data(Qt::CheckStateRole).toBool(),
        row[Column::PlaySound]->data(Qt::CheckStateRole).toBool(),
        row[Column::SoundPath]->data(Qt::UserRole).toString(),
        highlightColor};
}

// row into vector item
void BadgeHighlightModel::getRowFromItem(const HighlightBadge &item,
                                         std::vector<QStandardItem *> &row)
{
    using Column = BadgeHighlightModel::Column;

    row[Column::Badge]->setData(QVariant(item.badgePixmap()),
                                Qt::DecorationRole);
    setStringItem(row[Column::Badge], item.displayName());
    setBoolItem(row[Column::FlashTaskbar], item.hasAlert());
    setBoolItem(row[Column::PlaySound], item.hasSound());
    setFilePathItem(row[Column::SoundPath], item.getSoundUrl());
    setColorItem(row[Column::Color], *item.getColor());
}

}  // namespace chatterino
