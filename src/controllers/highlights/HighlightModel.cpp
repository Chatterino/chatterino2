#include "HighlightModel.hpp"

#include "Application.hpp"
#include "singletons/Settings.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
HighlightModel::HighlightModel(QObject *parent)
    : SignalVectorModel<HighlightPhrase>(7, parent)
{
}

// turn a vector item into a model row
HighlightPhrase HighlightModel::getItemFromRow(
    std::vector<QStandardItem *> &row, const HighlightPhrase &original)
{
    // key, alert, sound, regex, case-sensitivity

    return HighlightPhrase{row[0]->data(Qt::DisplayRole).toString(),
                           row[1]->data(Qt::CheckStateRole).toBool(),
                           row[2]->data(Qt::CheckStateRole).toBool(),
                           row[3]->data(Qt::CheckStateRole).toBool(),
                           row[4]->data(Qt::CheckStateRole).toBool(),
                           row[5]->data(Qt::DisplayRole).toString(),
                           row[6]->data(Qt::DecorationRole).value<QColor>()};
}

// turns a row in the model into a vector item
void HighlightModel::getRowFromItem(const HighlightPhrase &item,
                                    std::vector<QStandardItem *> &row)
{
    setStringItem(row[0], item.getPattern());
    setBoolItem(row[1], item.hasAlert());
    setBoolItem(row[2], item.hasSound());
    setBoolItem(row[3], item.isRegex());
    setBoolItem(row[4], item.isCaseSensitive());
    setStringItem(row[5], item.getSoundUrl().toString(), false, false);
    setColorItem(row[6], item.getColor());
}

void HighlightModel::afterInit()
{
    // Highlight settings for own username
    std::vector<QStandardItem *> usernameRow = this->createRow();
    setBoolItem(usernameRow[0], getSettings()->enableSelfHighlight.getValue(),
                true, false);
    usernameRow[0]->setData("Your username (automatic)", Qt::DisplayRole);
    setBoolItem(usernameRow[1],
                getSettings()->enableSelfHighlightTaskbar.getValue(), true,
                false);
    setBoolItem(usernameRow[2],
                getSettings()->enableSelfHighlightSound.getValue(), true,
                false);
    usernameRow[3]->setFlags(0);
    usernameRow[4]->setFlags(0);
    setStringItem(usernameRow[5],
                  getSettings()->selfHighlightSoundUrl.getValue(), false,
                  false);

    QString selfColor = getSettings()->selfHighlightColor.getValue();
    setColorItem(usernameRow[6], QColor(selfColor));

    this->insertCustomRow(usernameRow, 0);

    // Highlight settings for whispers
    std::vector<QStandardItem *> whisperRow = this->createRow();
    setBoolItem(whisperRow[0], getSettings()->enableWhisperHighlight.getValue(),
                true, false);
    whisperRow[0]->setData("Whispers", Qt::DisplayRole);
    setBoolItem(whisperRow[1],
                getSettings()->enableWhisperHighlightTaskbar.getValue(), true,
                false);
    setBoolItem(whisperRow[2],
                getSettings()->enableWhisperHighlightSound.getValue(), true,
                false);
    whisperRow[3]->setFlags(0);
    whisperRow[4]->setFlags(0);
    setStringItem(whisperRow[5],
                  getSettings()->whisperHighlightSoundUrl.getValue(), false,
                  false);

    QString whisperColor = getSettings()->whisperHighlightColor.getValue();
    setColorItem(whisperRow[6], QColor(whisperColor));

    this->insertCustomRow(whisperRow, 1);
}

void HighlightModel::customRowSetData(const std::vector<QStandardItem *> &row,
                                      int column, const QVariant &value,
                                      int role, int rowIndex)
{
    switch (column)
    {
        case 0:
        {
            if (role == Qt::CheckStateRole)
            {
                if (rowIndex == 0)
                {
                    getSettings()->enableSelfHighlight.setValue(value.toBool());
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
            // Regex --> empty element
        }
        break;
        case 4:
        {
            // Case-sensitivity --> empty
        }
        break;
        case 5:
        {
            // Custom sound file
            if (role == Qt::UserRole)
            {
                if (rowIndex == 0)
                {
                    getSettings()->selfHighlightSoundUrl.setValue(
                        value.toString());
                }
                else if (rowIndex == 1)
                {
                    getSettings()->whisperHighlightSoundUrl.setValue(
                        value.toString());
                }
            }
        }
        break;
        case 6:
        {
            // Custom color
            if (role == Qt::DecorationRole)
            {
                if (rowIndex == 0)
                {
                    getSettings()->selfHighlightColor.setValue(
                        value.value<QColor>().name());
                }
                else if (rowIndex == 1)
                {
                    getSettings()->whisperHighlightColor.setValue(
                        value.value<QColor>().name());
                }
            }
        }
        break;
    }
}

}  // namespace chatterino
