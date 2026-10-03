// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/ignores/IgnoredEmoteModel.hpp"

#include "util/StandardItemHelper.hpp"

namespace chatterino {

IgnoredEmoteModel::IgnoredEmoteModel(QObject *parent)
    : SignalVectorModel<IgnoredEmote>(2, parent)
{
}

IgnoredEmote IgnoredEmoteModel::getItemFromRow(
    std::vector<QStandardItem *> &row, const IgnoredEmote & /*original*/)
{
    return IgnoredEmote{row[0]->data(Qt::DisplayRole).toString(),
                        row[1]->data(Qt::CheckStateRole).toBool()};
}

void IgnoredEmoteModel::getRowFromItem(const IgnoredEmote &item,
                                       std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item.pattern);
    setBoolItem(row[1], item.regex);
}

}  // namespace chatterino
