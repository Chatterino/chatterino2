#include "AliasesModel.hpp"

#include "Application.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "singletons/Settings.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

AliasesModel::AliasesModel(QObject *parent)
    : SignalVectorModel<AliasesNamePtr>(2, parent)
{
}

// turn a vector item into a model row
AliasesNamePtr AliasesModel::getItemFromRow(std::vector<QStandardItem *> &row,
                                            const AliasesNamePtr &original)
{
    auto alias =
        std::make_shared<AliasesName>(row[0]->data(Qt::DisplayRole).toString(),
                                      row[1]->data(Qt::DisplayRole).toString());

    if (original->getName() != alias->getName())
    {
        qDebug() << "get id because name isn't equal. Original: "
                 << original->getName() << " New Alias: " << alias->getName();
        auto currentUser = getApp()->accounts->twitch.getCurrent();
        getHelix()->getUserByName(
            row[0]->data(Qt::DisplayRole).toString(),
            [&](const HelixUser &targetUser) {
                qDebug() << "here";  // << alias->getId();
                if (alias != nullptr)
                {
                    qDebug() << "is not null";
                }
                //alias.setId("penis" /*targetUser.id*/);
                qDebug() << "here2";
            },
            [] {});
    }

    return alias;
}

// turns a row in the model into a vector item
void AliasesModel::getRowFromItem(const AliasesNamePtr &item,
                                  std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item->getName());
    setStringItem(row[1], item->getReplace());
}

}  // namespace chatterino
