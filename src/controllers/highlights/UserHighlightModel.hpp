#pragma once

#include <QObject>

#include "common/SignalVectorModel.hpp"
#include "controllers/highlights/UserHighlight.hpp"

namespace chatterino {

class HighlightController;

class UserHighlightModel : public SignalVectorModel<UserHighlight>
{
    explicit UserHighlightModel(QObject *parent);

protected:
    // vector into model row
    virtual UserHighlight getItemFromRow(std::vector<QStandardItem *> &row,
                                         const UserHighlight &original) override;

    virtual void getRowFromItem(const UserHighlight &item,
                                std::vector<QStandardItem *> &row) override;

    friend class HighlightController;
};

}  // namespace chatterino
