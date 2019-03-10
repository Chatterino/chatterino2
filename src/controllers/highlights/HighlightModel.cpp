#include "HighlightModel.hpp"

#include "Application.hpp"
#include "singletons/Settings.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino
{
    // commandmodel
    HighlightModel::HighlightModel(QObject* parent)
        : SignalVectorModel<HighlightPhrase>(4, parent)
    {
    }

    // turn a vector item into a model row
    HighlightPhrase HighlightModel::getItemFromRow(
        std::vector<QStandardItem*>& row, const HighlightPhrase& original)
    {
        // key, alert, sound, regex

        return HighlightPhrase{row[0]->data(Qt::DisplayRole).toString(),
            row[1]->data(Qt::CheckStateRole).toBool(),
            row[2]->data(Qt::CheckStateRole).toBool(),
            row[3]->data(Qt::CheckStateRole).toBool()};
    }

    // turns a row in the model into a vector item
    void HighlightModel::getRowFromItem(
        const HighlightPhrase& item, std::vector<QStandardItem*>& row)
    {
        setStringItem(row[0], item.getPattern());
        setBoolItem(row[1], item.getAlert());
        setBoolItem(row[2], item.getSound());
        setBoolItem(row[3], item.isRegex());
    }

    void HighlightModel::afterInit()
    {
        std::vector<QStandardItem*> usernameRow = this->createRow();
        setBoolItem(usernameRow[0],
            getSettings()->enableSelfHighlight.getValue(), true, false);
        usernameRow[0]->setData("Your username (automatic)", Qt::DisplayRole);
        setBoolItem(usernameRow[1],
            getSettings()->enableSelfHighlightTaskbar.getValue(), true, false);
        setBoolItem(usernameRow[2],
            getSettings()->enableSelfHighlightSound.getValue(), true, false);
        usernameRow[3]->setFlags(0);
        this->insertCustomRow(usernameRow, 0);
        std::vector<QStandardItem*> whisperRow = this->createRow();
        setBoolItem(whisperRow[0],
            getSettings()->enableWhisperHighlight.getValue(), true, false);
        whisperRow[0]->setData("Whispers", Qt::DisplayRole);
        setBoolItem(whisperRow[1],
            getSettings()->enableWhisperHighlightTaskbar.getValue(), true,
            false);
        setBoolItem(whisperRow[2],
            getSettings()->enableWhisperHighlightSound.getValue(), true, false);
        whisperRow[3]->setFlags(0);
        this->insertCustomRow(whisperRow, 1);
    }

    void HighlightModel::customRowSetData(
        const std::vector<QStandardItem*>& row, int column,
        const QVariant& value, int role, int rowIndex)
    {
        switch (column)
        {
            case 0:
            {
                if (role == Qt::CheckStateRole)
                {
                    if (rowIndex == 0)
                    {
                        getSettings()->enableSelfHighlight.setValue(
                            value.toBool());
                    }
                    else if (rowIndex == 1)
                    {
                        getSettings()->enableWhisperHighlight.setValue(
                            value.toBool());
                    }
                }
            }
            break;
            case 1:
            {
                if (role == Qt::CheckStateRole)
                {
                    if (rowIndex == 0)
                    {
                        getSettings()->enableSelfHighlightTaskbar.setValue(
                            value.toBool());
                    }
                    else if (rowIndex == 1)
                    {
                        getSettings()->enableWhisperHighlightTaskbar.setValue(
                            value.toBool());
                    }
                }
            }
            break;
            case 2:
            {
                if (role == Qt::CheckStateRole)
                {
                    if (rowIndex == 0)
                    {
                        getSettings()->enableSelfHighlightSound.setValue(
                            value.toBool());
                    }
                    else if (rowIndex == 1)
                    {
                        getSettings()->enableWhisperHighlightSound.setValue(
                            value.toBool());
                    }
                }
            }
            break;
            case 3:
            {
                // empty element
            }
            break;
        }
    }

}  // namespace chatterino
