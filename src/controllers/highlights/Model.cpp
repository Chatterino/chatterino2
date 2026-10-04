// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/Model.hpp"

#include "common/QLogging.hpp"
#include "common/SignalVectorModel.hpp"
#include "controllers/highlights/types/AnyHighlight.hpp"  // IWYU pragma: keep
#include "util/StandardItemHelper.hpp"

#include <QPalette>
#include <QPointer>

namespace chatterino::highlights {

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
const auto &LOG = chatterinoHighlights;

}  // namespace

Model::Model(QObject *parent)
    : SignalVectorModel<AnyHighlight>(Column::COUNT, parent)
{
}

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Model::updateRow(const AnyHighlight &highlight,
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

    /*
    if (const auto *h = std::get_if<BadgeHighlight>(&highlight))
    {
        // TODO: If the badge fails to load, it would be nice if the loading icon could be replaced with an error icon of some kind
        getApp()->getTwitchBadges()->getBadgeIcon(
            h->getBadgeName(),
            [model = QPointer(this), id = h->getID()](
                const QString &name, const std::shared_ptr<QIcon> &icon) {
                NOTE!!!!!!!!! This code doesn't work, it can override highlights that come after it.
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
    */

    setStringItem(row[Column::Name], getName(highlight), false);
    setStringItem(row[Column::Sound], "");  // TODO: include full URL?
    row[Column::Sound]->setData(soundIcon, Qt::DecorationRole);
}

AnyHighlight Model::getItemFromRow(std::vector<QStandardItem *> &row,
                                   const AnyHighlight &original)
{
    (void)original;  // unused

    auto item = get<AnyHighlight>(row[Column::Enabled]->data(DATA_ROLE));

    this->updateRow(item, row);

    return item;
}

void Model::getRowFromItem(const AnyHighlight &item,
                           std::vector<QStandardItem *> &row)
{
    row[Column::Enabled]->setData(QVariant::fromValue(item), DATA_ROLE);
    row[Column::Enabled]->setData(QVariant::fromValue(getID(item)), ID_ROLE);

    this->updateRow(item, row);
}
// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

}  // namespace chatterino::highlights
