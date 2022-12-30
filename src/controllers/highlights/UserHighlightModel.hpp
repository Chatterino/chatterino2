#pragma once

#include "common/SignalVectorModel.hpp"

#include <QObject>

namespace chatterino {

class HighlightController;
class HighlightPhrase;

class UserHighlightModel : public SignalVectorModel<HighlightPhrase>
{
public:
    explicit UserHighlightModel(QObject *parent);

protected:
    // vector into model row
    virtual HighlightPhrase getItemFromRow(
        std::vector<QStandardItem *> &row,
        const HighlightPhrase &original) override;

    virtual void getRowFromItem(const HighlightPhrase &item,
                                std::vector<QStandardItem *> &row) override;
};

}  // namespace chatterino
