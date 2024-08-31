#include "controllers/highlights/HighlightModel.hpp"

#include "Application.hpp"
#include "common/SignalVectorModel.hpp"
#include "controllers/highlights/HighlightPhrase.hpp"
#include "providers/colors/ColorProvider.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

// commandmodel
HighlightModel::HighlightModel(QObject *parent)
    : SignalVectorModel<HighlightPhrase>(Column::COUNT, parent)
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
        row[Column::ShowInMentions]->data(Qt::CheckStateRole).toBool(),
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
    setBoolItem(row[Column::ShowInMentions], item.showInMentions());
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
    setBoolItem(usernameRow[Column::ShowInMentions],
                getSettings()->showSelfHighlightInMentions.getValue(), true,
                false);
    setBoolItem(usernameRow[Column::FlashTaskbar],
                getSettings()->enableSelfHighlightTaskbar.getValue(), true,
                false);
    setBoolItem(usernameRow[Column::PlaySound],
                getSettings()->enableSelfHighlightSound.getValue(), true,
                false);
    usernameRow[Column::UseRegex]->setFlags({});
    usernameRow[Column::CaseSensitive]->setFlags({});

    QUrl selfSound = QUrl(getSettings()->selfHighlightSoundUrl.getValue());
    setFilePathItem(usernameRow[Column::SoundPath], selfSound, false);

    auto selfColor = ColorProvider::instance().color(ColorType::SelfHighlight);
    setColorItem(usernameRow[Column::Color], *selfColor, false);

    this->insertCustomRow(usernameRow, HighlightRowIndexes::SelfHighlightRow);

    // Highlight settings for whispers
    std::vector<QStandardItem *> whisperRow = this->createRow();
    setBoolItem(whisperRow[Column::Pattern],
                getSettings()->enableWhisperHighlight.getValue(), true, false);
    whisperRow[Column::Pattern]->setData("Whispers", Qt::DisplayRole);
    whisperRow[Column::ShowInMentions]->setFlags({});  // We have /whispers
    setBoolItem(whisperRow[Column::FlashTaskbar],
                getSettings()->enableWhisperHighlightTaskbar.getValue(), true,
                false);
    setBoolItem(whisperRow[Column::PlaySound],
                getSettings()->enableWhisperHighlightSound.getValue(), true,
                false);
    whisperRow[Column::UseRegex]->setFlags({});
    whisperRow[Column::CaseSensitive]->setFlags({});

    QUrl whisperSound =
        QUrl(getSettings()->whisperHighlightSoundUrl.getValue());
    setFilePathItem(whisperRow[Column::SoundPath], whisperSound, false);

    auto whisperColor = ColorProvider::instance().color(ColorType::Whisper);
    setColorItem(whisperRow[Column::Color], *whisperColor, false);

    this->insertCustomRow(whisperRow, HighlightRowIndexes::WhisperRow);

    // Highlight settings for subscription messages
    std::vector<QStandardItem *> subRow = this->createRow();
    setBoolItem(subRow[Column::Pattern],
                getSettings()->enableSubHighlight.getValue(), true, false);
    subRow[Column::Pattern]->setData("Subscriptions", Qt::DisplayRole);
    subRow[Column::ShowInMentions]->setFlags({});
    setBoolItem(subRow[Column::FlashTaskbar],
                getSettings()->enableSubHighlightTaskbar.getValue(), true,
                false);
    setBoolItem(subRow[Column::PlaySound],
                getSettings()->enableSubHighlightSound.getValue(), true, false);
    subRow[Column::UseRegex]->setFlags({});
    subRow[Column::CaseSensitive]->setFlags({});

    QUrl subSound = QUrl(getSettings()->subHighlightSoundUrl.getValue());
    setFilePathItem(subRow[Column::SoundPath], subSound, false);

    auto subColor = ColorProvider::instance().color(ColorType::Subscription);
    setColorItem(subRow[Column::Color], *subColor, false);

    this->insertCustomRow(subRow, HighlightRowIndexes::SubRow);

    // Highlight settings for redeemed highlight messages
    std::vector<QStandardItem *> redeemedRow = this->createRow();
    setBoolItem(redeemedRow[Column::Pattern],
                getSettings()->enableRedeemedHighlight.getValue(), true, false);
    redeemedRow[Column::Pattern]->setData(
        "Highlights redeemed with Channel Points", Qt::DisplayRole);
    redeemedRow[Column::ShowInMentions]->setFlags({});
    //    setBoolItem(redeemedRow[Column::FlashTaskbar],
    //                getSettings()->enableRedeemedHighlightTaskbar.getValue(), true,
    //                false);
    //    setBoolItem(redeemedRow[Column::PlaySound],
    //                getSettings()->enableRedeemedHighlightSound.getValue(), true,
    //                false);
    redeemedRow[Column::FlashTaskbar]->setFlags({});
    redeemedRow[Column::PlaySound]->setFlags({});
    redeemedRow[Column::UseRegex]->setFlags({});
    redeemedRow[Column::CaseSensitive]->setFlags({});
    redeemedRow[Column::SoundPath]->setFlags(Qt::NoItemFlags);

    auto RedeemedColor =
        ColorProvider::instance().color(ColorType::RedeemedHighlight);
    setColorItem(redeemedRow[Column::Color], *RedeemedColor, false);

    this->insertCustomRow(redeemedRow, HighlightRowIndexes::RedeemedRow);

    // Highlight settings for first messages
    std::vector<QStandardItem *> firstMessageRow = this->createRow();
    setBoolItem(firstMessageRow[Column::Pattern],
                getSettings()->enableFirstMessageHighlight.getValue(), true,
                false);
    firstMessageRow[Column::Pattern]->setData("First Messages",
                                              Qt::DisplayRole);
    firstMessageRow[Column::ShowInMentions]->setFlags({});
    //    setBoolItem(firstMessageRow[Column::FlashTaskbar],
    //                getSettings()->enableFirstMessageHighlightTaskbar.getValue(),
    //                true, false);
    //    setBoolItem(firstMessageRow[Column::PlaySound],
    //                getSettings()->enableFirstMessageHighlightSound.getValue(),
    //                true, false);
    firstMessageRow[Column::FlashTaskbar]->setFlags({});
    firstMessageRow[Column::PlaySound]->setFlags({});
    firstMessageRow[Column::UseRegex]->setFlags({});
    firstMessageRow[Column::CaseSensitive]->setFlags({});
    firstMessageRow[Column::SoundPath]->setFlags(Qt::NoItemFlags);

    auto FirstMessageColor =
        ColorProvider::instance().color(ColorType::FirstMessageHighlight);
    setColorItem(firstMessageRow[Column::Color], *FirstMessageColor, false);

    this->insertCustomRow(firstMessageRow,
                          HighlightRowIndexes::FirstMessageRow);

    // Highlight settings for hype chats
    std::vector<QStandardItem *> elevatedMessageRow = this->createRow();
    setBoolItem(elevatedMessageRow[Column::Pattern],
                getSettings()->enableElevatedMessageHighlight.getValue(), true,
                false);
    elevatedMessageRow[Column::Pattern]->setData("Hype Chats", Qt::DisplayRole);
    elevatedMessageRow[Column::ShowInMentions]->setFlags({});
    //    setBoolItem(elevatedMessageRow[Column::FlashTaskbar],
    //                getSettings()->enableElevatedMessageHighlightTaskbar.getValue(),
    //                true, false);
    //    setBoolItem(elevatedMessageRow[Column::PlaySound],
    //                getSettings()->enableElevatedMessageHighlightSound.getValue(),
    //                true, false);
    elevatedMessageRow[Column::FlashTaskbar]->setFlags({});
    elevatedMessageRow[Column::PlaySound]->setFlags({});
    elevatedMessageRow[Column::UseRegex]->setFlags({});
    elevatedMessageRow[Column::CaseSensitive]->setFlags({});
    elevatedMessageRow[Column::SoundPath]->setFlags(Qt::NoItemFlags);

    auto elevatedMessageColor =
        ColorProvider::instance().color(ColorType::ElevatedMessageHighlight);
    setColorItem(elevatedMessageRow[Column::Color], *elevatedMessageColor,
                 false);

    this->insertCustomRow(elevatedMessageRow,
                          HighlightRowIndexes::ElevatedMessageRow);

    // Highlight settings for reply threads
    std::vector<QStandardItem *> threadMessageRow = this->createRow();
    setBoolItem(threadMessageRow[Column::Pattern],
                getSettings()->enableThreadHighlight.getValue(), true, false);
    threadMessageRow[Column::Pattern]->setData("Subscribed Reply Threads",
                                               Qt::DisplayRole);
    setBoolItem(threadMessageRow[Column::ShowInMentions],
                getSettings()->showThreadHighlightInMentions.getValue(), true,
                false);
    setBoolItem(threadMessageRow[Column::FlashTaskbar],
                getSettings()->enableThreadHighlightTaskbar.getValue(), true,
                false);
    setBoolItem(threadMessageRow[Column::PlaySound],
                getSettings()->enableThreadHighlightSound.getValue(), true,
                false);
    threadMessageRow[Column::UseRegex]->setFlags({});
    threadMessageRow[Column::CaseSensitive]->setFlags({});

    QUrl threadMessageSound =
        QUrl(getSettings()->threadHighlightSoundUrl.getValue());
    setFilePathItem(threadMessageRow[Column::SoundPath], threadMessageSound,
                    false);

    auto threadMessageColor =
        ColorProvider::instance().color(ColorType::ThreadMessageHighlight);
    setColorItem(threadMessageRow[Column::Color], *threadMessageColor, false);

    this->insertCustomRow(threadMessageRow,
                          HighlightRowIndexes::ThreadMessageRow);

    // Highlight settings for automod caught messages
    const std::vector<QStandardItem *> automodRow = this->createRow();
    setBoolItem(automodRow[Column::Pattern],
                getSettings()->enableAutomodHighlight.getValue(), true, false);
    setBoolItem(automodRow[Column::ShowInMentions],
                getSettings()->showAutomodInMentions.getValue(), true, false);
    automodRow[Column::Pattern]->setData("AutoMod Caught Messages",
                                         Qt::DisplayRole);
    setBoolItem(automodRow[Column::FlashTaskbar],
                getSettings()->enableAutomodHighlightTaskbar.getValue(), true,
                false);
    setBoolItem(automodRow[Column::PlaySound],
                getSettings()->enableAutomodHighlightSound.getValue(), true,
                false);
    automodRow[Column::UseRegex]->setFlags({});
    automodRow[Column::CaseSensitive]->setFlags({});

    const auto automodSound =
        QUrl(getSettings()->automodHighlightSoundUrl.getValue());
    setFilePathItem(automodRow[Column::SoundPath], automodSound, false);
    auto automodColor =
        ColorProvider::instance().color(ColorType::AutomodHighlight);
    setColorItem(automodRow[Column::Color], *automodColor, false);

    this->insertCustomRow(automodRow, HighlightRowIndexes::AutomodRow);
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
                if (rowIndex == HighlightRowIndexes::SelfHighlightRow)
                {
                    getSettings()->enableSelfHighlight.setValue(value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::WhisperRow)
                {
                    getSettings()->enableWhisperHighlight.setValue(
                        value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::SubRow)
                {
                    getSettings()->enableSubHighlight.setValue(value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::RedeemedRow)
                {
                    getSettings()->enableRedeemedHighlight.setValue(
                        value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::FirstMessageRow)
                {
                    getSettings()->enableFirstMessageHighlight.setValue(
                        value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::ElevatedMessageRow)
                {
                    getSettings()->enableElevatedMessageHighlight.setValue(
                        value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::ThreadMessageRow)
                {
                    getSettings()->enableThreadHighlight.setValue(
                        value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::AutomodRow)
                {
                    getSettings()->enableAutomodHighlight.setValue(
                        value.toBool());
                }
            }
        }
        break;
        case Column::ShowInMentions: {
            if (role == Qt::CheckStateRole)
            {
                if (rowIndex == HighlightRowIndexes::SelfHighlightRow)
                {
                    getSettings()->showSelfHighlightInMentions.setValue(
                        value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::ThreadMessageRow)
                {
                    getSettings()->showThreadHighlightInMentions.setValue(
                        value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::AutomodRow)
                {
                    getSettings()->showAutomodInMentions.setValue(
                        value.toBool());
                }
            }
        }
        break;
        case Column::FlashTaskbar: {
            if (role == Qt::CheckStateRole)
            {
                if (rowIndex == HighlightRowIndexes::SelfHighlightRow)
                {
                    getSettings()->enableSelfHighlightTaskbar.setValue(
                        value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::WhisperRow)
                {
                    getSettings()->enableWhisperHighlightTaskbar.setValue(
                        value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::SubRow)
                {
                    getSettings()->enableSubHighlightTaskbar.setValue(
                        value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::RedeemedRow)
                {
                    // getSettings()->enableRedeemedHighlightTaskbar.setValue(
                    //     value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::FirstMessageRow)
                {
                    // getSettings()->enableFirstMessageHighlightTaskbar.setValue(
                    //     value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::ElevatedMessageRow)
                {
                    // getSettings()
                    //     ->enableElevatedMessageHighlightTaskbar.setvalue(
                    //         value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::ThreadMessageRow)
                {
                    getSettings()->enableThreadHighlightTaskbar.setValue(
                        value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::AutomodRow)
                {
                    getSettings()->enableAutomodHighlightTaskbar.setValue(
                        value.toBool());
                }
            }
        }
        break;
        case Column::PlaySound: {
            if (role == Qt::CheckStateRole)
            {
                if (rowIndex == HighlightRowIndexes::SelfHighlightRow)
                {
                    getSettings()->enableSelfHighlightSound.setValue(
                        value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::WhisperRow)
                {
                    getSettings()->enableWhisperHighlightSound.setValue(
                        value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::SubRow)
                {
                    getSettings()->enableSubHighlightSound.setValue(
                        value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::RedeemedRow)
                {
                    // getSettings()->enableRedeemedHighlightSound.setValue(
                    //     value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::FirstMessageRow)
                {
                    // getSettings()->enableFirstMessageHighlightSound.setValue(
                    //     value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::ElevatedMessageRow)
                {
                    // getSettings()->enableElevatedMessageHighlightSound.setValue(
                    //     value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::ThreadMessageRow)
                {
                    getSettings()->enableThreadHighlightSound.setValue(
                        value.toBool());
                }
                else if (rowIndex == HighlightRowIndexes::AutomodRow)
                {
                    getSettings()->enableAutomodHighlightSound.setValue(
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
                if (rowIndex == HighlightRowIndexes::SelfHighlightRow)
                {
                    getSettings()->selfHighlightSoundUrl.setValue(
                        value.toString());
                }
                else if (rowIndex == HighlightRowIndexes::WhisperRow)
                {
                    getSettings()->whisperHighlightSoundUrl.setValue(
                        value.toString());
                }
                else if (rowIndex == HighlightRowIndexes::SubRow)
                {
                    getSettings()->subHighlightSoundUrl.setValue(
                        value.toString());
                }
                else if (rowIndex == HighlightRowIndexes::ThreadMessageRow)
                {
                    getSettings()->threadHighlightSoundUrl.setValue(
                        value.toString());
                }
                else if (rowIndex == HighlightRowIndexes::AutomodRow)
                {
                    getSettings()->automodHighlightSoundUrl.setValue(
                        value.toString());
                }
            }
        }
        break;
        case Column::Color: {
            // Custom color
            if (role == Qt::DecorationRole)
            {
                const auto setColor = [&](auto &setting, ColorType ty) {
                    auto color = value.value<QColor>();
                    setting.setValue(color.name(QColor::HexArgb));
                };

                if (rowIndex == HighlightRowIndexes::SelfHighlightRow)
                {
                    setColor(getSettings()->selfHighlightColor,
                             ColorType::SelfHighlight);
                }
                else if (rowIndex == HighlightRowIndexes::WhisperRow)
                {
                    setColor(getSettings()->whisperHighlightColor,
                             ColorType::Whisper);
                }
                else if (rowIndex == HighlightRowIndexes::SubRow)
                {
                    setColor(getSettings()->subHighlightColor,
                             ColorType::Subscription);
                }
                else if (rowIndex == HighlightRowIndexes::RedeemedRow)
                {
                    setColor(getSettings()->redeemedHighlightColor,
                             ColorType::RedeemedHighlight);
                }
                else if (rowIndex == HighlightRowIndexes::FirstMessageRow)
                {
                    setColor(getSettings()->firstMessageHighlightColor,
                             ColorType::FirstMessageHighlight);
                }
                else if (rowIndex == HighlightRowIndexes::ElevatedMessageRow)
                {
                    setColor(getSettings()->elevatedMessageHighlightColor,
                             ColorType::ElevatedMessageHighlight);
                }
                else if (rowIndex == HighlightRowIndexes::ThreadMessageRow)
                {
                    setColor(getSettings()->threadHighlightColor,
                             ColorType::ThreadMessageHighlight);
                }
                else if (rowIndex == HighlightRowIndexes::AutomodRow)
                {
                    setColor(getSettings()->automodHighlightColor,
                             ColorType::AutomodHighlight);
                }
            }
        }
        break;
    }

    getApp()->getWindows()->forceLayoutChannelViews();
}

}  // namespace chatterino
