#pragma once

#include <QObject>

#include "common/SignalVectorModel.hpp"
#include "controllers/filters/FilterRecord.hpp"

namespace chatterino {

class FilterModel : public SignalVectorModel<FilterRecordPtr>
{
public:
    explicit FilterModel(QObject *parent);

protected:
    // turn a vector item into a model row
    virtual FilterRecordPtr getItemFromRow(
        std::vector<QStandardItem *> &row,
        const FilterRecordPtr &original) override;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const FilterRecordPtr &item,
                                std::vector<QStandardItem *> &row) override;
};

}  // namespace chatterino
