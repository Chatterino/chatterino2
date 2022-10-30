#pragma once

#include <QObject>

#include "common/SignalVectorModel.hpp"
#include "controllers/nicknames/Nickname.hpp"

namespace chatterino {

class NicknamesModel : public SignalVectorModel<Nickname>
{
public:
    explicit NicknamesModel(QObject *parent);

protected:
    // turn a vector item into a model row
    virtual Nickname getItemFromRow(std::vector<QStandardItem *> &row,
                                    const Nickname &original) override;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const Nickname &item,
                                std::vector<QStandardItem *> &row) override;
};

}  // namespace chatterino
