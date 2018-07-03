#pragma once

#include <QObject>

#include "common/SignalVectorModel.hpp"
#include "controllers/moderationactions/ModerationAction.hpp"

namespace chatterino {

class ModerationActions;

class ModerationActionModel : public SignalVectorModel<ModerationAction>
{
public:
    explicit ModerationActionModel(QObject *parent);

protected:
    // turn a vector item into a model row
    virtual ModerationAction getItemFromRow(std::vector<QStandardItem *> &row,
                                            const ModerationAction &original) override;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const ModerationAction &item,
                                std::vector<QStandardItem *> &row) override;

    friend class HighlightController;

    friend class ModerationActions;
};

}  // namespace chatterino
