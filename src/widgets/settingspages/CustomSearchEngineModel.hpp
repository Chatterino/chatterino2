// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "common/CustomSearchEngine.hpp"
#include "common/SignalVectorModel.hpp"

#include <QObject>

namespace chatterino {

class CustomSearchEngineModel : public SignalVectorModel<CustomSearchEngine>
{
public:
    explicit CustomSearchEngineModel(QObject *parent);

    enum Column {
        Name,
        URL,
        COUNT,
    };

protected:
    // turn a vector item into a model row
    CustomSearchEngine getItemFromRow(std::vector<QStandardItem *> &row,
                                      const CustomSearchEngine &original) override;

    // turns a row in the model into a vector item
    void getRowFromItem(const CustomSearchEngine &item,
                        std::vector<QStandardItem *> &row) override;

    friend class GeneralPage;
};

}  // namespace chatterino
