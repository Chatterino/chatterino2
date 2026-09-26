// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/Model.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "common/SignalVectorModel.hpp"
#include "controllers/highlights/types/All.hpp"  // IWYU pragma: keep
#include "debug/AssertInGuiThread.hpp"
#include "providers/twitch/TwitchBadges.hpp"
#include "util/PostToThread.hpp"
#include "util/StandardItemHelper.hpp"

#include <QPalette>
#include <QPointer>

namespace chatterino::highlights {

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
const auto &LOG = chatterinoHighlights;

}  // namespace

Model::Model(QObject *parent)
    : SignalVectorModel<AllHighlights>(Column::COUNT, parent)
{
}

void Model::updateRow(const AllHighlights &highlight,
                      std::vector<QStandardItem *> &row)
{
    QIcon enabledIcon{":/buttons/checkmark-square.svg"};
    QIcon disabledIcon{":/buttons/dismiss-square.svg"};

    auto soundIcon = [highlight] {
        if (shouldPlaySound(highlight))
        {
            return QIcon{":/buttons/music-note-1.svg"};
        }

        return QIcon{":/buttons/speaker-mute.svg"};
    }();

    auto enabled = isEnabled(highlight);

    QPalette palette;

    if (auto error = getError(highlight); !error.isEmpty())
    {
        // Highlight has an error
        row[Column::Name]->setData(error, Qt::ToolTipRole);
        row[Column::Enabled]->setData(error, Qt::ToolTipRole);

        QFont f;
        f.setStrikeOut(true);
        row[Column::Name]->setData(f, Qt::FontRole);

        row[Column::Enabled]->setData("Error", Qt::EditRole);
        row[Column::Enabled]->setData(disabledIcon, Qt::DecorationRole);
    }
    else
    {
        row[Column::Name]->setData(QString{}, Qt::ToolTipRole);
        row[Column::Enabled]->setData(QString{}, Qt::ToolTipRole);

        QFont f;
        row[Column::Name]->setData(f, Qt::FontRole);

        if (enabled)
        {
            // Highlight is enabled
            row[Column::Enabled]->setData("Enabled", Qt::EditRole);

            // Undim name
            const auto &b = palette.text();
            row[Column::Name]->setData(b, Qt::ForegroundRole);
            row[Column::Enabled]->setData(enabledIcon, Qt::DecorationRole);
        }
        else
        {
            // Highlight is disabled
            row[Column::Enabled]->setData("Disabled", Qt::EditRole);

            // Dim name
            const auto &b = palette.placeholderText();
            row[Column::Name]->setData(b, Qt::ForegroundRole);
            row[Column::Enabled]->setData(disabledIcon, Qt::DecorationRole);
        }
    }

    row[Column::Name]->setData(getIcon(highlight), Qt::DecorationRole);

    if (const auto *h = std::get_if<BadgeHighlight>(&highlight))
    {
        getApp()->getTwitchBadges()->getBadgeIcon(
            h->getBadgeName(),
            [model = QPointer(this), id = h->getID()](
                const QString &name, const std::shared_ptr<QIcon> &icon) {
                (void)name;  // unused

                runInGuiThread([model, id, icon] {
                    if (!model)
                    {
                        return;
                    }

                    auto matches = model->match(
                        model->index(0, highlights::Model::Column::Enabled),
                        highlights::Model::ID_ROLE, QVariant::fromValue(id), 1,
                        Qt::MatchExactly | Qt::MatchWrap);
                    if (matches.isEmpty())
                    {
                        qCWarning(LOG)
                            << "Attempted to set badge icon for" << id
                            << "but it is missing. Was it removed?";
                        return;
                    }
                    auto matchingCell = matches.first();
                    assert(matchingCell.isValid());
                    auto nameCell = matchingCell.siblingAtColumn(Column::Name);
                    assert(nameCell.isValid());
                    model->setData(nameCell, *icon, Qt::DecorationRole);
                });
            });
    }

    setStringItem(row[Column::Name], getName(highlight), false);
    setStringItem(row[Column::Sound], "");  // TODO: include full URL?
    row[Column::Sound]->setData(soundIcon, Qt::DecorationRole);
}

AllHighlights Model::getItemFromRow(std::vector<QStandardItem *> &row,
                                    const AllHighlights &original)
{
    (void)original;  // unused

    auto item = get<AllHighlights>(row[Column::Enabled]->data(DATA_ROLE));

    this->updateRow(item, row);

    return item;
}

void Model::getRowFromItem(const AllHighlights &item,
                           std::vector<QStandardItem *> &row)
{
    row[Column::Enabled]->setData(QVariant::fromValue(item), DATA_ROLE);
    row[Column::Enabled]->setData(QVariant::fromValue(getID(item)), ID_ROLE);

    this->updateRow(item, row);
}

}  // namespace chatterino::highlights
