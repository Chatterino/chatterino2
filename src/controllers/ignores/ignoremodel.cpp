#include "ignoremodel.hpp"

#include "application.hpp"
#include "singletons/settingsmanager.hpp"
#include "util/standarditemhelper.hpp"

namespace chatterino {
namespace controllers {
namespace ignores {

// commandmodel
IgnoreModel::IgnoreModel(QObject *parent)
    : util::SignalVectorModel<IgnorePhrase>(2, parent)
{
}

// turn a vector item into a model row
IgnorePhrase IgnoreModel::getItemFromRow(std::vector<QStandardItem *> &row)
{
    // key, regex

    return IgnorePhrase{row[0]->data(Qt::DisplayRole).toString(),
                        row[1]->data(Qt::CheckStateRole).toBool()};
}

// turns a row in the model into a vector item
void IgnoreModel::getRowFromItem(const IgnorePhrase &item, std::vector<QStandardItem *> &row)
{
    util::setStringItem(row[0], item.getPattern());
    util::setBoolItem(row[1], item.isRegex());
}

}  // namespace ignores
}  // namespace controllers
}  // namespace chatterino
