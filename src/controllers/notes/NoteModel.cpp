#include "NoteModel.hpp"

#include "Application.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "singletons/Settings.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

NotesModel::NotesModel(QObject *parent)
    : SignalVectorModel<Note>(4, parent)
{
}

// turn a vector item into a model row
Note NotesModel::getItemFromRow(std::vector<QStandardItem *> &row,
                                const Note &original)
{
    return Note{row[0]->data(Qt::DisplayRole).toString(),
                row[1]->data(Qt::DisplayRole).toString()};
}

// turns a row in the model into a vector item
void NotesModel::getRowFromItem(const Note &item,
                                std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item.id());
    setStringItem(row[1], item.note());
}

}  // namespace chatterino
