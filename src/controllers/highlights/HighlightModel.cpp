#include "HighlightModel.hpp"

#include "Application.hpp"
#include "singletons/Settings.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
HighlightModel::HighlightModel(QObject *parent)
    : SignalVectorModel<HighlightPhrase>(4, parent)
{
}

// turn a vector item into a model row
HighlightPhrase HighlightModel::getItemFromRow(
    std::vector<QStandardItem *> &row, const HighlightPhrase &original)
{
    // key, alert, sound, regex

    return HighlightPhrase{row[0]->data(Qt::DisplayRole).toString(),
                           row[1]->data(Qt::CheckStateRole).toBool(),
                           row[2]->data(Qt::CheckStateRole).toBool(),
                           row[3]->data(Qt::CheckStateRole).toBool()};
}

// turns a row in the model into a vector item
void HighlightModel::getRowFromItem(const HighlightPhrase &item,
                                    std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item.getPattern());
    setBoolItem(row[1], item.getAlert());
    setBoolItem(row[2], item.getSound());
    setBoolItem(row[3], item.isRegex());
}

void HighlightModel::afterInit()
{
    std::vector<QStandardItem *> row = this->createRow();
    setBoolItem(row[0], getSettings()->enableHighlightsSelf.getValue(), true,
                false);
    row[0]->setData("Your username (automatic)", Qt::DisplayRole);
    setBoolItem(row[1], getSettings()->enableHighlightTaskbar.getValue(), true,
                false);
    setBoolItem(row[2], getSettings()->enableHighlightSound.getValue(), true,
                false);
    row[3]->setFlags(0);
    this->insertCustomRow(row, 0);
}

void HighlightModel::customRowSetData(const std::vector<QStandardItem *> &row,
                                      int column, const QVariant &value,
                                      int role)
{
    switch (column) {
        case 0: {
            if (role == Qt::CheckStateRole) {
                getSettings()->enableHighlightsSelf.setValue(value.toBool());
            }
        } break;
        case 1: {
            if (role == Qt::CheckStateRole) {
                getSettings()->enableHighlightTaskbar.setValue(value.toBool());
            }
        } break;
        case 2: {
            if (role == Qt::CheckStateRole) {
                getSettings()->enableHighlightSound.setValue(value.toBool());
            }
        } break;
        case 3: {
            // empty element
        } break;
    }
}

}  // namespace chatterino
