#include "HotkeyModel.hpp"

#include "common/QLogging.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

HotkeyModel::HotkeyModel(QObject *parent)
    : SignalVectorModel<std::shared_ptr<Hotkey>>(2, parent)
{
}

// turn a vector item into a model row
std::shared_ptr<Hotkey> HotkeyModel::getItemFromRow(
    std::vector<QStandardItem *> &row, const std::shared_ptr<Hotkey> &original)
{
    return original;
}

// turns a row in the model into a vector item
void HotkeyModel::getRowFromItem(const std::shared_ptr<Hotkey> &item,
                                 std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item->name(), false);
    row[0]->setData(QFont("Segoe UI", 10), Qt::FontRole);

    setStringItem(row[1], item->toPortableString(), false);
    row[1]->setData(QFont("Segoe UI", 10), Qt::FontRole);
}

int HotkeyModel::beforeInsert(const std::shared_ptr<Hotkey> &item,
                              std::vector<QStandardItem *> &row,
                              int proposedIndex)
{
    const auto category = item->getCategory();
    if (this->categoryCount_[category]++ == 0)
    {
        auto newRow = this->createRow();

        setStringItem(newRow[0], category, false, false);
        newRow[0]->setData(QFont("Segoe UI Light", 16), Qt::FontRole);

        // make sure category headers aren't editable
        for (unsigned long i = 1; i < newRow.size(); i++)
        {
            setStringItem(newRow[i], "", false, false);
        }

        this->insertCustomRow(std::move(newRow), proposedIndex);

        return proposedIndex + 1;
    }

    return proposedIndex;
}

void HotkeyModel::afterRemoved(const std::shared_ptr<Hotkey> &item,
                               std::vector<QStandardItem *> &row, int index)
{
    auto it = this->categoryCount_.find(item->getCategory());
    assert(it != this->categoryCount_.end());

    if (it->second <= 1)
    {
        this->categoryCount_.erase(it);
        this->removeCustomRow(index - 1);
    }
    else
    {
        it->second--;
    }
}

}  // namespace chatterino
