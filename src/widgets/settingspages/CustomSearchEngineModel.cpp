// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/settingspages/CustomSearchEngineModel.hpp"

#include "util/StandardItemHelper.hpp"

namespace chatterino {

CustomSearchEngineModel::CustomSearchEngineModel(QObject *parent)
    : SignalVectorModel<CustomSearchEngine>(Column::COUNT, parent)
{
}

CustomSearchEngine CustomSearchEngineModel::getItemFromRow(
    std::vector<QStandardItem *> &row, const CustomSearchEngine & /*original*/)
{
    auto name = row[Column::Name]->data(Qt::DisplayRole).toString();
    auto url = row[Column::URL]->data(Qt::DisplayRole).toString();
    return {name, url};
}

void CustomSearchEngineModel::getRowFromItem(const CustomSearchEngine &item,
                                             std::vector<QStandardItem *> &row)
{
    setStringItem(row[Column::Name], item.name);
    setStringItem(row[Column::URL], item.url);
}

}  // namespace chatterino
