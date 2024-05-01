#pragma once

#include "common/SignalVectorModel.hpp"

#include <QObject>

namespace chatterino {

class ModerationAction;

class ModerationActionModel : public SignalVectorModel<ModerationAction>
{
public:
    explicit ModerationActionModel(QObject *parent);

    enum Column {
        Command = 0,
        Icon = 1,
    };

protected:
    // turn a vector item into a model row
    ModerationAction getItemFromRow(std::vector<QStandardItem *> &row,
                                    const ModerationAction &original) override;

    // turns a row in the model into a vector item
    void getRowFromItem(const ModerationAction &item,
                        std::vector<QStandardItem *> &row) override;

    friend class HighlightController;
};

}  // namespace chatterino
