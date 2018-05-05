#include "highlightmodel.hpp"

#include "application.hpp"
#include "singletons/settingsmanager.hpp"
#include "util/standarditemhelper.hpp"

namespace chatterino {
namespace controllers {
namespace highlights {

// commandmodel
HighlightModel::HighlightModel(QObject *parent)
    : util::SignalVectorModel<HighlightPhrase>(4, parent)
{
    //    auto app = getApp();

    //    std::vector<QStandardItem *> row = this->createRow();

    //    util::setBoolItem(row[0], app->settings->enableHighlightsSelf.getValue(), true, false);
    //    util::setBoolItem(row[1], app->settings->enableHighlightsSelf.getValue(), true, false);
    //    util::setBoolItem(row[2], app->settings->enableHighlightsSelf.getValue(), true, false);
    //    row[0]->setData("Your name (automatic)", Qt::DisplayRole);

    //    this->insertCustomRow(row, 0);
}

// app->settings->highlightProperties.setValue(phrases);
// app->settings->enableHighlightsSelf.setValue(
//     model->item(0, 0)->data(Qt::CheckStateRole).toBool());
// app->settings->enableHighlightTaskbar.setValue(
//     model->item(0, 1)->data(Qt::CheckStateRole).toBool());
// app->settings->enableHighlightSound.setValue(
//     model->item(0, 2)->data(Qt::CheckStateRole).toBool());

// turn a vector item into a model row
HighlightPhrase HighlightModel::getItemFromRow(std::vector<QStandardItem *> &row)
{
    // key, alert, sound, regex

    return HighlightPhrase{
        row[0]->data(Qt::DisplayRole).toString(), row[1]->data(Qt::CheckStateRole).toBool(),
        row[2]->data(Qt::CheckStateRole).toBool(), row[3]->data(Qt::CheckStateRole).toBool()};
}

// turns a row in the model into a vector item
void HighlightModel::getRowFromItem(const HighlightPhrase &item, std::vector<QStandardItem *> &row)
{
    util::setStringItem(row[0], item.key);
    util::setBoolItem(row[1], item.alert);
    util::setBoolItem(row[2], item.sound);
    util::setBoolItem(row[3], item.regex);
}

void HighlightModel::afterInit()
{
    std::vector<QStandardItem *> row = this->createRow();
    util::setBoolItem(row[0], getApp()->settings->enableHighlightsSelf.getValue(), true, false);
    row[0]->setData("Your username (automatic)", Qt::DisplayRole);
    util::setBoolItem(row[1], getApp()->settings->enableHighlightTaskbar.getValue(), true, false);
    util::setBoolItem(row[2], getApp()->settings->enableHighlightSound.getValue(), true, false);
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
