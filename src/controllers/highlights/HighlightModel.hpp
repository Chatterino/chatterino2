#pragma once

#include <QObject>

#include "controllers/highlights/HighlightPhrase.hpp"
#include "util/SignalVectorModel.hpp"

namespace chatterino {
namespace controllers {
namespace highlights {

class HighlightController;

class HighlightModel : public util::SignalVectorModel<HighlightPhrase>
{
    explicit HighlightModel(QObject *parent);

protected:
    // turn a vector item into a model row
    virtual HighlightPhrase getItemFromRow(std::vector<QStandardItem *> &row,
                                           const HighlightPhrase &original) override;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const HighlightPhrase &item,
                                std::vector<QStandardItem *> &row) override;

    virtual void afterInit() override;

    virtual void customRowSetData(const std::vector<QStandardItem *> &row, int column,
                                  const QVariant &value, int role) override;

    friend class HighlightController;
};

}  // namespace highlights
}  // namespace controllers
}  // namespace chatterino
