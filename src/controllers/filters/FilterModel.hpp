#pragma once

#include <QObject>

#include "common/SignalVectorModel.hpp"
#include "controllers/filters/FilterRecord.hpp"

namespace chatterino {

class FilterModel : public SignalVectorModel<FilterRecord>
{
public:
    explicit FilterModel(QObject *parent);

protected:
    // turn a vector item into a model row
    virtual FilterRecord getItemFromRow(std::vector<QStandardItem *> &row,
                                        const FilterRecord &original) override;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const FilterRecord &item,
                                std::vector<QStandardItem *> &row) override;
};

}  // namespace chatterino
