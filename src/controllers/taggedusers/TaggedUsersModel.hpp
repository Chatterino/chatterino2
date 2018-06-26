#pragma once

#include "controllers/taggedusers/TaggedUser.hpp"
#include "common/SignalVectorModel.hpp"

namespace chatterino {

class TaggedUsersController;

class TaggedUsersModel : public SignalVectorModel<TaggedUser>
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

}  // namespace chatterino
