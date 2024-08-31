#include "controllers/hotkeys/HotkeyModel.hpp"

#include "common/QLogging.hpp"
#include "controllers/hotkeys/Hotkey.hpp"
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
    QFont font("Segoe UI", 10);

    if (!item->validAction())
    {
        font.setStrikeOut(true);
    }

    setStringItem(row[0], item->name(), false);
    row[0]->setData(font, Qt::FontRole);

    setStringItem(row[1], item->toString(), false);
    row[1]->setData(font, Qt::FontRole);
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

    auto [currentCategoryModelIndex, nextCategoryModelIndex] =
        this->getCurrentAndNextCategoryModelIndex(category);

    if (nextCategoryModelIndex != -1 && proposedIndex >= nextCategoryModelIndex)
    {
        // The proposed index would have landed under the wrong category, we offset by -1 to compensate
        return proposedIndex - 1;
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

std::tuple<int, int> HotkeyModel::getCurrentAndNextCategoryModelIndex(
    const QString &category) const
{
    int modelIndex = 0;

    int currentCategoryModelIndex = -1;
    int nextCategoryModelIndex = -1;

    for (const auto &row : this->rows())
    {
        if (row.isCustomRow)
        {
            QString customRowValue =
                row.items[0]->data(Qt::EditRole).toString();
            if (currentCategoryModelIndex != -1)
            {
                nextCategoryModelIndex = modelIndex;
                break;
            }
            if (customRowValue == category)
            {
                currentCategoryModelIndex = modelIndex;
            }
        }

        modelIndex += 1;
    }

    return {currentCategoryModelIndex, nextCategoryModelIndex};
}

}  // namespace chatterino
