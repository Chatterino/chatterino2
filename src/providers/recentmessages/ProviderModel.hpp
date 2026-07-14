// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "common/SignalVectorModel.hpp"
#include "providers/recentmessages/Provider.hpp"

#include <QObject>

namespace chatterino::recentmessages {

class ProviderModel : public SignalVectorModel<Provider>
{
public:
    explicit ProviderModel(QObject *parent);

    [[nodiscard]] bool canRemoveRow(int row) const;
    bool removeRows(int row, int count,
                    const QModelIndex &parent = QModelIndex()) override;

protected:
    Provider getItemFromRow(std::vector<QStandardItem *> &row,
                            const Provider &original) override;
    void getRowFromItem(const Provider &item,
                        std::vector<QStandardItem *> &row) override;

private:
    enum Column {
        Enabled,
        Url,
        Count,
    };
};

}  // namespace chatterino::recentmessages
