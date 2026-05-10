#include "controllers/highlights/Model.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "common/SignalVectorModel.hpp"
#include "controllers/highlights/types/All.hpp"  // IWYU pragma: keep
#include "providers/twitch/TwitchBadges.hpp"
#include "util/StandardItemHelper.hpp"
#include "util/Variant.hpp"

#include <qnamespace.h>

namespace chatterino::highlights {

namespace {

void updateRow(const AllHighlights &highlight,
               std::vector<QStandardItem *> &row)
{
    using Column = Model::Column;

    auto soundPixmap = std::visit(
        [](auto &&h) {
            if (h.willPlayCustomSound())
            {
                return QIcon{":/buttons/music-note.svg"};
            }

            if (h.willPlayAnySound())
            {
                return QIcon{":/buttons/music-note-2.svg"};
            }

            return QIcon{":/buttons/speaker-mute.svg"};
        },
        highlight);

    if (std::visit(
            [](auto &&v) {
                return v.isEnabled();
            },
            highlight))
    {
        setStringItem(row[Column::Enabled], "Enabled", false);
    }
    else
    {
        setStringItem(row[Column::Enabled], "Disabled", false);
    }

    std::visit(
        variant::Overloaded{
            [row](const BadgeHighlight &h) {
                getApp()->getTwitchBadges()->getBadgeIcon(
                    h.getBadgeName(),
                    [row](const QString &name,
                          const std::shared_ptr<QIcon> &icon) {
                        (void)name;  // unused
                        row[Column::Name]->setData(*icon, Qt::DecorationRole);
                    });
            },
            [row](auto &&v) {
                row[Column::Name]->setData(v.getType(), Qt::DecorationRole);
            },
        },
        highlight);

    setStringItem(row[Column::Name],
                  std::visit(
                      variant::Overloaded{
                          [](auto &&v) {
                              return v.getName();
                          },
                      },
                      highlight),
                  false);
    setStringItem(row[Column::Sound], "a");  // TODO: include full URL?
    row[Column::Sound]->setData(soundPixmap, Qt::DecorationRole);
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
    auto id = std::visit(
        [](auto &&h) {
            return h.getID();
        },
        item);
    row[Column::Enabled]->setData(QVariant::fromValue(id), ID_ROLE);

    updateRow(item, row);
}

}  // namespace chatterino::highlights
