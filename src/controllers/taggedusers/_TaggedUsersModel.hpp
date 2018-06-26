#pragma once

#include "controllers/taggedusers/taggeduser.hpp"
#include "util/signalvectormodel.hpp"

namespace chatterino {
namespace controllers {
namespace taggedusers {

class TaggedUsersController;

class TaggedUsersModel : public util::SignalVectorModel<TaggedUser>
{
    explicit TaggedUsersModel(QObject *parent);

protected:
    // turn a vector item into a model row
    virtual TaggedUser getItemFromRow(std::vector<QStandardItem *> &row,
                                      const TaggedUser &original) override;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const TaggedUser &item, std::vector<QStandardItem *> &row) override;

    virtual void afterInit() override;

    //    virtual void customRowSetData(const std::vector<QStandardItem *> &row, int column,
    //                                  const QVariant &value, int role) override;

    friend class TaggedUsersController;
};

}  // namespace taggedusers
}  // namespace controllers
}  // namespace chatterino
