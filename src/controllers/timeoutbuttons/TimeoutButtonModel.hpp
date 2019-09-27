#pragma once

#include "common/SignalVectorModel.hpp"
#include "controllers/timeoutbuttons/TimeoutButton.hpp"
#include "controllers/timeoutbuttons/TimeoutButtonController.hpp"

namespace chatterino {

class TimeoutButton;
class TimeoutButtonController;

class TimeoutButtonModel : public SignalVectorModel<TimeoutButton>
{
public:
    TimeoutButtonModel(QObject *parent);

protected:
    // turn a vector item into a model row
    virtual TimeoutButton getItemFromRow(
        std::vector<QStandardItem *> &row,
        const TimeoutButton &original) override;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const TimeoutButton &item,
                                std::vector<QStandardItem *> &row) override;
    friend class TimeoutButtonController;

private:
};

}  // namespace chatterino
