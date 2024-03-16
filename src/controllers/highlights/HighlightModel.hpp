#pragma once

#include "common/SignalVectorModel.hpp"

#include <QObject>

namespace chatterino {

class HighlightPhrase;

class HighlightModel : public SignalVectorModel<HighlightPhrase>
{
public:
    explicit HighlightModel(QObject *parent);

    // Used here, in HighlightingPage and in UserHighlightModel
    enum Column {
        Pattern = 0,
        ShowInMentions = 1,
        FlashTaskbar = 2,
        UseRegex = 3,
        CaseSensitive = 4,
        PlaySound = 5,
        SoundPath = 6,
        Color = 7,
        COUNT  // keep this as last member of enum
    };

    enum HighlightRowIndexes {
        SelfHighlightRow = 0,
        WhisperRow = 1,
        SubRow = 2,
        RedeemedRow = 3,
        FirstMessageRow = 4,
        ElevatedMessageRow = 5,
        ThreadMessageRow = 6,
        AutomodRow = 7,
    };

    enum UserHighlightRowIndexes {
        SelfMessageRow = 0,
    };

protected:
    // turn a vector item into a model row
    HighlightPhrase getItemFromRow(std::vector<QStandardItem *> &row,
                                   const HighlightPhrase &original) override;

    // turns a row in the model into a vector item
    void getRowFromItem(const HighlightPhrase &item,
                        std::vector<QStandardItem *> &row) override;

    void afterInit() override;

    void customRowSetData(const std::vector<QStandardItem *> &row, int column,
                          const QVariant &value, int role,
                          int rowIndex) override;
};

}  // namespace chatterino
