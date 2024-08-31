#pragma once

#include "common/SignalVectorModel.hpp"

#include <QObject>

namespace chatterino {

class IgnorePhrase;

class IgnoreModel : public SignalVectorModel<IgnorePhrase>
{
public:
    explicit IgnoreModel(QObject *parent);

protected:
    // turn a vector item into a model row
    IgnorePhrase getItemFromRow(std::vector<QStandardItem *> &row,
                                const IgnorePhrase &original) override;

    // turns a row in the model into a vector item
    void getRowFromItem(const IgnorePhrase &item,
                        std::vector<QStandardItem *> &row) override;
};

}  // namespace chatterino
