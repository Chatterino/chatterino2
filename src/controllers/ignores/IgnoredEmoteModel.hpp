// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "common/SignalVectorModel.hpp"
#include "controllers/ignores/IgnoredEmote.hpp"

namespace chatterino {

class IgnoredEmoteModel : public SignalVectorModel<IgnoredEmote>
{
public:
    explicit IgnoredEmoteModel(QObject *parent);

protected:
    IgnoredEmote getItemFromRow(std::vector<QStandardItem *> &row,
                                const IgnoredEmote &original) override;
    void getRowFromItem(const IgnoredEmote &item,
                        std::vector<QStandardItem *> &row) override;
};

}  // namespace chatterino
