#include "controllers/highlights/Model.hpp"

#include "Application.hpp"
#include "common/SignalVectorModel.hpp"
#include "controllers/highlights/types/All.hpp"  // IWYU pragma: keep
#include "providers/twitch/TwitchBadges.hpp"
#include "util/StandardItemHelper.hpp"
#include "util/Variant.hpp"

#include <QPalette>

namespace chatterino::highlights {

namespace {

void updateRow(const AllHighlights &highlight,
               std::vector<QStandardItem *> &row)
{
    using Column = Model::Column;

    auto soundIcon = [highlight] {
        if (willPlayCustomSound(highlight))
        {
            return QIcon{":/buttons/music-note.svg"};
        }

        if (shouldPlaySound(highlight))
        {
            return QIcon{":/buttons/music-note-2.svg"};
        }

        return QIcon{":/buttons/speaker-mute.svg"};
    }();

    auto enabled = isEnabled(highlight);

    QPalette palette;

    if (enabled)
    {
        setStringItem(row[Column::Enabled], "Enabled", false);

        // Undim name
        const auto &b = palette.text();
        row[Column::Name]->setData(b, Qt::ForegroundRole);
    }
    else
    {
        setStringItem(row[Column::Enabled], "Disabled", false);

        // Dim name
        const auto &b = palette.placeholderText();
        row[Column::Name]->setData(b, Qt::ForegroundRole);
    }

    row[Column::Name]->setData(getIcon(highlight), Qt::DecorationRole);

    // TODO: use an "if type is BadgeHighlight"
    std::visit(variant::Overloaded{
                   [row](const BadgeHighlight &h) {
                       getApp()->getTwitchBadges()->getBadgeIcon(
                           h.getBadgeName(),
                           [row](const QString &name,
                                 const std::shared_ptr<QIcon> &icon) {
                               (void)name;  // unused
                               row[Column::Name]->setData(*icon,
                                                          Qt::DecorationRole);
                           });
                   },
                   [row](auto &&v) {},
               },
               highlight);

    setStringItem(row[Column::Name], getName(highlight), false);
    setStringItem(row[Column::Sound], "");  // TODO: include full URL?
    row[Column::Sound]->setData(soundIcon, Qt::DecorationRole);
}

}  // namespace

Model::Model(QObject *parent)
    : SignalVectorModel<AllHighlights>(Column::COUNT, parent)
{
}

AllHighlights Model::getItemFromRow(std::vector<QStandardItem *> &row,
                                    const AllHighlights &original)
{
    (void)original;  // unused

    auto item = get<AllHighlights>(row[Column::Enabled]->data(DATA_ROLE));

    updateRow(item, row);

    return item;
}

void Model::getRowFromItem(const AllHighlights &item,
                           std::vector<QStandardItem *> &row)
{
    row[Column::Enabled]->setData(QVariant::fromValue(item), DATA_ROLE);
    row[Column::Enabled]->setData(QVariant::fromValue(getID(item)), ID_ROLE);

    updateRow(item, row);
}

}  // namespace chatterino::highlights
