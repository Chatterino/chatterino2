// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "providers/recentmessages/ProviderModel.hpp"

#include "util/StandardItemHelper.hpp"

#include <utility>

namespace chatterino::recentmessages {

ProviderModel::ProviderModel(QObject *parent)
    : SignalVectorModel(Column::Count, parent)
{
}

bool ProviderModel::canRemoveRow(int row) const
{
    if (row < 0 || std::cmp_greater_equal(row, this->rows().size()))
    {
        return false;
    }

    const auto &original = this->rows()[row].original;
    return original && !original->isBuiltIn();
}

bool ProviderModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (count != 1 || !this->canRemoveRow(row))
    {
        return false;
    }

    return SignalVectorModel::removeRows(row, count, parent);
}

Provider ProviderModel::getItemFromRow(std::vector<QStandardItem *> &row,
                                       const Provider &original)
{
    // keep the stable ID when a built-in provider is enabled or disabled
    return {
        row[Column::Url]->data(Qt::DisplayRole).toString().trimmed(),
        row[Column::Enabled]->data(Qt::CheckStateRole).toBool(),
        original.id(),
    };
}

void ProviderModel::getRowFromItem(const Provider &item,
                                   std::vector<QStandardItem *> &row)
{
    setBoolItem(row[Column::Enabled], item.enabled());
    setStringItem(row[Column::Url], item.url(), !item.isBuiltIn());
}

}  // namespace chatterino::recentmessages
