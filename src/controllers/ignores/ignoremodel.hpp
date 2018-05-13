#pragma once

#include <QObject>

#include "controllers/ignores/ignorephrase.hpp"
#include "util/signalvectormodel.hpp"

namespace chatterino {
namespace controllers {
namespace ignores {

class IgnoreController;

class IgnoreModel : public util::SignalVectorModel<IgnorePhrase>
{
    explicit IgnoreModel(QObject *parent);

protected:
    // turn a vector item into a model row
    virtual IgnorePhrase getItemFromRow(std::vector<QStandardItem *> &row) override;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const IgnorePhrase &item,
                                std::vector<QStandardItem *> &row) override;

    friend class IgnoreController;
};

}  // namespace ignores
}  // namespace controllers
}  // namespace chatterino
