#include "TaggedUsersModel.hpp"

#include "Application.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
TaggedUsersModel::TaggedUsersModel(QObject *parent)
    : SignalVectorModel<TaggedUser>(1, parent)
{
}

// turn a vector item into a model row
TaggedUser TaggedUsersModel::getItemFromRow(std::vector<QStandardItem *> &row,
                                            const TaggedUser &original)
{
    return original;
}

// turns a row in the model into a vector item
void TaggedUsersModel::getRowFromItem(const TaggedUser &item,
                                      std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item.getName());
}

void TaggedUsersModel::afterInit()
{
    //    std::vector<QStandardItem *> row = this->createRow();
    //    setBoolItem(row[0],
    //    getApp()->settings->enableHighlightsSelf.getValue(), true, false);
    //    row[0]->setData("Your username (automatic)", Qt::DisplayRole);
    //    setBoolItem(row[1],
    //    getApp()->settings->enableHighlightTaskbar.getValue(), true, false);
    //    setBoolItem(row[2],
    //    getApp()->settings->enableHighlightSound.getValue(), true, false);
    //    row[3]->setFlags(0); this->insertCustomRow(row, 0);
}

// void TaggedUserModel::customRowSetData(const std::vector<QStandardItem *>
// &row, int column,
//                                       const QVariant &value, int role)
//{
//    switch (column) {
//        case 0: {
//            if (role == Qt::CheckStateRole) {
//                getApp()->settings->enableHighlightsSelf.setValue(value.toBool());
//            }
//        } break;
//        case 1: {
//            if (role == Qt::CheckStateRole) {
//                getApp()->settings->enableHighlightTaskbar.setValue(value.toBool());
//            }
//        } break;
//        case 2: {
//            if (role == Qt::CheckStateRole) {
//                getApp()->settings->enableHighlightSound.setValue(value.toBool());
//            }
//        } break;
//        case 3: {
//            // empty element
//        } break;
//    }
//}

}  // namespace chatterino
