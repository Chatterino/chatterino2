#pragma once

#include <QObject>

#include "common/SignalVectorModel.hpp"
#include "controllers/notes/Note.hpp"

namespace chatterino {

class NotesModel : public SignalVectorModel<Note>
{
public:
    explicit NotesModel(QObject *parent);

protected:
    // turn a vector item into a model row
    virtual Note getItemFromRow(std::vector<QStandardItem *> &row,
                                    const NOte &original) override;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const Note &item,
                                std::vector<QStandardItem *> &row) override;
};

}  // namespace chatterino
