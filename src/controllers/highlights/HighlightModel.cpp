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
    // In order for old messages to update their highlight color, we need to
    // update the highlight color here.
    auto highlightColor = original.getColor();
    *highlightColor =
        row[Column::Color]->data(Qt::DecorationRole).value<QColor>();

    return HighlightPhrase{
        row[Column::Pattern]->data(Qt::DisplayRole).toString(),
        row[Column::FlashTaskbar]->data(Qt::CheckStateRole).toBool(),
        row[Column::PlaySound]->data(Qt::CheckStateRole).toBool(),
        row[Column::UseRegex]->data(Qt::CheckStateRole).toBool(),
        row[Column::CaseSensitive]->data(Qt::CheckStateRole).toBool(),
        row[Column::SoundPath]->data(Qt::UserRole).toString(),
        highlightColor};
}

// turns a row in the model into a vector item
void HighlightModel::getRowFromItem(const HighlightPhrase &item,
                                    std::vector<QStandardItem *> &row)
{
    setStringItem(row[Column::Pattern], item.getPattern());
    setBoolItem(row[Column::FlashTaskbar], item.hasAlert());
    setBoolItem(row[Column::PlaySound], item.hasSound());
    setBoolItem(row[Column::UseRegex], item.isRegex());
    setBoolItem(row[Column::CaseSensitive], item.isCaseSensitive());
    setFilePathItem(row[Column::SoundPath], item.getSoundUrl());
    setColorItem(row[Column::Color], *item.getColor());
}

void HighlightModel::afterInit()
{
    // Highlight settings for own username
    std::vector<QStandardItem *> usernameRow = this->createRow();
    setBoolItem(usernameRow[Column::Pattern],
                getSettings()->enableSelfHighlight.getValue(), true, false);
    usernameRow[Column::Pattern]->setData("Your username (automatic)",
                                          Qt::DisplayRole);
    setBoolItem(usernameRow[Column::FlashTaskbar],
                getSettings()->enableSelfHighlightTaskbar.getValue(), true,
                false);
    setBoolItem(usernameRow[Column::PlaySound],
                getSettings()->enableSelfHighlightSound.getValue(), true,
                false);
    usernameRow[Column::UseRegex]->setFlags(0);
    usernameRow[Column::CaseSensitive]->setFlags(0);

    QUrl selfSound = QUrl(getSettings()->selfHighlightSoundUrl.getValue());
    setFilePathItem(usernameRow[Column::SoundPath], selfSound);

    auto selfColor = ColorProvider::instance().color(ColorType::SelfHighlight);
    setColorItem(usernameRow[Column::Color], *selfColor);

    this->insertCustomRow(usernameRow, 0);

    // Highlight settings for whispers
    std::vector<QStandardItem *> whisperRow = this->createRow();
    setBoolItem(whisperRow[Column::Pattern],
                getSettings()->enableWhisperHighlight.getValue(), true, false);
    whisperRow[Column::Pattern]->setData("Whispers", Qt::DisplayRole);
    setBoolItem(whisperRow[Column::FlashTaskbar],
                getSettings()->enableWhisperHighlightTaskbar.getValue(), true,
                false);
    setBoolItem(whisperRow[Column::PlaySound],
                getSettings()->enableWhisperHighlightSound.getValue(), true,
                false);
    whisperRow[Column::UseRegex]->setFlags(0);
    whisperRow[Column::CaseSensitive]->setFlags(0);

    QUrl whisperSound =
        QUrl(getSettings()->whisperHighlightSoundUrl.getValue());
    setFilePathItem(whisperRow[Column::SoundPath], whisperSound);

    auto whisperColor = ColorProvider::instance().color(ColorType::Whisper);
    setColorItem(whisperRow[Column::Color], *whisperColor);

    this->insertCustomRow(whisperRow, 1);

    // Highlight settings for subscription messages
    std::vector<QStandardItem *> subRow = this->createRow();
    setBoolItem(subRow[Column::Pattern],
                getSettings()->enableSubHighlight.getValue(), true, false);
    subRow[Column::Pattern]->setData("Subscriptions", Qt::DisplayRole);
    setBoolItem(subRow[Column::FlashTaskbar],
                getSettings()->enableSubHighlightTaskbar.getValue(), true,
                false);
    setBoolItem(subRow[Column::PlaySound],
                getSettings()->enableSubHighlightSound.getValue(), true, false);
    subRow[Column::UseRegex]->setFlags(0);
    subRow[Column::CaseSensitive]->setFlags(0);

    QUrl subSound = QUrl(getSettings()->subHighlightSoundUrl.getValue());
    setFilePathItem(subRow[Column::SoundPath], subSound);

    auto subColor = ColorProvider::instance().color(ColorType::Subscription);
    setColorItem(subRow[Column::Color], *subColor);

    this->insertCustomRow(subRow, 2);
}

void HighlightModel::customRowSetData(const std::vector<QStandardItem *> &row,
                                      int column, const QVariant &value,
                                      int role, int rowIndex)
{
    switch (column)
    {
        case Column::Pattern: {
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
                else if (rowIndex == 2)
                {
                    getSettings()->enableSubHighlight.setValue(value.toBool());
                }
            }
        }
        break;
        case Column::FlashTaskbar: {
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
                else if (rowIndex == 2)
                {
                    getSettings()->enableSubHighlightTaskbar.setValue(
                        value.toBool());
                }
            }
        }
        break;
        case Column::PlaySound: {
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
                else if (rowIndex == 2)
                {
                    getSettings()->enableSubHighlightSound.setValue(
                        value.toBool());
                }
            }
        }
        break;
        case Column::UseRegex: {
            // Regex --> empty
        }
        break;
        case Column::CaseSensitive: {
            // Case-sensitivity --> empty
        }
        break;
        case Column::SoundPath: {
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
                else if (rowIndex == 2)
                {
                    getSettings()->subHighlightSoundUrl.setValue(
                        value.toString());
                }
            }
        }
        break;
        case Column::Color: {
            // Custom color
            if (role == Qt::DecorationRole)
            {
                auto colorName = value.value<QColor>().name(QColor::HexArgb);
                if (rowIndex == 0)
                {
                    getSettings()->selfHighlightColor.setValue(colorName);
                }
                else if (rowIndex == 1)
                {
                    getSettings()->whisperHighlightColor.setValue(colorName);
                }
                else if (rowIndex == 2)
                {
                    getSettings()->subHighlightColor.setValue(colorName);
                }
            }
        }
        break;
    }
}

}  // namespace chatterino
