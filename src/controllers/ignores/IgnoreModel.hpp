#pragma once

#include <QObject>

#include "common/SignalVectorModel.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"

namespace chatterino {

class IgnoreModel : public SignalVectorModel<IgnorePhrase>
{
public:
    explicit IgnoreModel(QObject *parent);

protected:
    // turn a vector item into a model row
    virtual IgnorePhrase getItemFromRow(std::vector<QStandardItem *> &row,
                                        const IgnorePhrase &original) override;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const IgnorePhrase &item,
                                std::vector<QStandardItem *> &row) override;
};

}  // namespace chatterino
