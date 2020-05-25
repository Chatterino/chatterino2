#include "BadgeHighlightModel.hpp"

#include "Application.hpp"
#include "common/GlobalBadges.hpp"
#include "messages/Emote.hpp"
#include "singletons/Settings.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
BadgeHighlightModel::BadgeHighlightModel(QObject *parent)
    : SignalVectorModel<HighlightBadge>(5, parent)
{
    GlobalBadges::instance();
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
    using QIconPtr = std::shared_ptr<QIcon>;
    using Column = BadgeHighlightModel::Column;

    setStringItem(row[Column::Badge], item.displayName());
    setBoolItem(row[Column::FlashTaskbar], item.hasAlert());
    setBoolItem(row[Column::PlaySound], item.hasSound());
    setFilePathItem(row[Column::SoundPath], item.getSoundUrl());
    setColorItem(row[Column::Color], *item.getColor());

    GlobalBadges::instance()->getBadgeIcon(
        item.identifier(),
        [item, row](QString /*identifier*/, const QIconPtr pixmap) {
            row[Column::Badge]->setData(QVariant(*pixmap), Qt::DecorationRole);
        });
}

}  // namespace chatterino
