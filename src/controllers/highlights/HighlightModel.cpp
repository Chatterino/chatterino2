#include "HighlightModel.hpp"

#include "Application.hpp"
#include "singletons/SettingsManager.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {
namespace controllers {
namespace highlights {

// commandmodel
HighlightModel::HighlightModel(QObject *parent)
    : util::SignalVectorModel<HighlightPhrase>(4, parent)
{
}

// turn a vector item into a model row
HighlightPhrase HighlightModel::getItemFromRow(std::vector<QStandardItem *> &row,
                                               const HighlightPhrase &original)
{
    // key, alert, sound, regex

    return HighlightPhrase{
        row[0]->data(Qt::DisplayRole).toString(), row[1]->data(Qt::CheckStateRole).toBool(),
        row[2]->data(Qt::CheckStateRole).toBool(), row[3]->data(Qt::CheckStateRole).toBool()};
}

// turns a row in the model into a vector item
void HighlightModel::getRowFromItem(const HighlightPhrase &item, std::vector<QStandardItem *> &row)
{
    util::setStringItem(row[0], item.getPattern());
    util::setBoolItem(row[1], item.getAlert());
    util::setBoolItem(row[2], item.getSound());
    util::setBoolItem(row[3], item.isRegex());
}

void HighlightModel::afterInit()
{
    std::vector<QStandardItem *> row = this->createRow();
    util::setBoolItem(row[0], getApp()->settings->enableHighlightsSelf.getValue(), true, false);
    row[0]->setData("Your username (automatic)", Qt::DisplayRole);
    util::setBoolItem(row[1], getApp()->settings->enableHighlightTaskbar.getValue(), true, false);
    util::setBoolItem(row[2], getApp()->settings->enableHighlightSound.getValue(), true, false);
    row[3]->setFlags(0);
    this->insertCustomRow(row, 0);
}

void HighlightModel::customRowSetData(const std::vector<QStandardItem *> &row, int column,
                                      const QVariant &value, int role)
{
    switch (column) {
        case 0: {
            if (role == Qt::CheckStateRole) {
                getApp()->settings->enableHighlightsSelf.setValue(value.toBool());
            }
        } break;
        case 1: {
            if (role == Qt::CheckStateRole) {
                getApp()->settings->enableHighlightTaskbar.setValue(value.toBool());
            }
        } break;
        case 2: {
            if (role == Qt::CheckStateRole) {
                getApp()->settings->enableHighlightSound.setValue(value.toBool());
            }
        } break;
        case 3: {
            // empty element
        } break;
    }
}

}  // namespace highlights
}  // namespace controllers
}  // namespace chatterino
