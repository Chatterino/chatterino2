#pragma once

#include "common/SignalVectorModel.hpp"
#include "controllers/highlights/HighlightModel.hpp"

#include <QObject>

namespace chatterino {

class HighlightController;
class HighlightPhrase;

class UserHighlightModel : public SignalVectorModel<HighlightPhrase>
{
public:
    using Column = HighlightModel::Column;

    explicit UserHighlightModel(QObject *parent);

protected:
    // vector into model row
    HighlightPhrase getItemFromRow(std::vector<QStandardItem *> &row,
                                   const HighlightPhrase &original) override;

    void getRowFromItem(const HighlightPhrase &item,
                        std::vector<QStandardItem *> &row) override;

    void afterInit() override;

    void customRowSetData(const std::vector<QStandardItem *> &row, int column,
                          const QVariant &value, int role,
                          int rowIndex) override;
};

}  // namespace chatterino
