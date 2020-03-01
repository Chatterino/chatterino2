#pragma once

#include <QObject>

#include "common/SignalVectorModel.hpp"
#include "controllers/highlights/HighlightBlacklistUser.hpp"

namespace chatterino {

class HighlightController;

class HighlightBlacklistModel : public SignalVectorModel<HighlightBlacklistUser>
{
public:
    explicit HighlightBlacklistModel(QObject *parent);

    enum Column {
        Pattern = 0,
        UseRegex = 1,
    };

protected:
    // turn a vector item into a model row
    virtual HighlightBlacklistUser getItemFromRow(
        std::vector<QStandardItem *> &row,
        const HighlightBlacklistUser &original) override;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const HighlightBlacklistUser &item,
                                std::vector<QStandardItem *> &row) override;
};

}  // namespace chatterino
