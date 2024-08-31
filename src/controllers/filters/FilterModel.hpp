#pragma once

#include "common/SignalVectorModel.hpp"

#include <QObject>

namespace chatterino {

class FilterRecord;
using FilterRecordPtr = std::shared_ptr<FilterRecord>;

class FilterModel : public SignalVectorModel<FilterRecordPtr>
{
public:
    explicit FilterModel(QObject *parent);

protected:
    // turn a vector item into a model row
    FilterRecordPtr getItemFromRow(std::vector<QStandardItem *> &row,
                                   const FilterRecordPtr &original) override;

    // turns a row in the model into a vector item
    void getRowFromItem(const FilterRecordPtr &item,
                        std::vector<QStandardItem *> &row) override;
};

}  // namespace chatterino
