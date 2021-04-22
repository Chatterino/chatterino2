#pragma once

#include <QObject>

#include "common/SignalVectorModel.hpp"
#include "controllers/highlights/HighlightPhrase.hpp"

namespace chatterino {

class HighlightModel : public SignalVectorModel<HighlightPhrase>
{
public:
    explicit HighlightModel(QObject *parent);

    // Used here, in HighlightingPage and in UserHighlightModel
    enum Column {
        Pattern = 0,
        ShowInMentions = 1,
        FlashTaskbar = 2,
        PlaySound = 3,
        UseRegex = 4,
        CaseSensitive = 5,
        SoundPath = 6,
        Color = 7,
        COUNT  // keep this as last member of enum
    };

    constexpr static int WHISPER_ROW = 1;

protected:
    // turn a vector item into a model row
    virtual HighlightPhrase getItemFromRow(
        std::vector<QStandardItem *> &row,
        const HighlightPhrase &original) override;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const HighlightPhrase &item,
                                std::vector<QStandardItem *> &row) override;

    virtual void afterInit() override;

    virtual void customRowSetData(const std::vector<QStandardItem *> &row,
                                  int column, const QVariant &value, int role,
                                  int rowIndex) override;
};

}  // namespace chatterino
