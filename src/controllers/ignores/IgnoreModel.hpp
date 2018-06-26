#pragma once

#include <QObject>

#include "controllers/ignores/IgnorePhrase.hpp"
#include "common/SignalVectorModel.hpp"

namespace chatterino {

class IgnoreController;

class IgnoreModel : public SignalVectorModel<IgnorePhrase>
{
    explicit IgnoreModel(QObject *parent);

protected:
    // turn a vector item into a model row
    virtual IgnorePhrase getItemFromRow(std::vector<QStandardItem *> &row,
                                        const IgnorePhrase &original) override;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const IgnorePhrase &item,
                                std::vector<QStandardItem *> &row) override;

    friend class IgnoreController;
};

}  // namespace chatterino
