#pragma once

#include <QObject>

#include "common/SignalVectorModel.hpp"
#include "controllers/aliases/AliasesName.hpp"

namespace chatterino {

class AliasesModel : public SignalVectorModel<AliasesName>
{
public:
    explicit AliasesModel(QObject *parent);

protected:
    // turn a vector item into a model row
    virtual AliasesName getItemFromRow(std::vector<QStandardItem *> &row,
                                       const AliasesName &original) override;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const AliasesName &item,
                                std::vector<QStandardItem *> &row) override;
};

}  // namespace chatterino
