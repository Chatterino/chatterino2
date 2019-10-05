#pragma once

#include <QObject>

#include "common/SignalVectorModel.hpp"
#include "controllers/highlights/HighlightPhrase.hpp"

namespace chatterino {

class HighlightController;

class HighlightModel : public SignalVectorModel<HighlightPhrase>
{
    explicit HighlightModel(QObject *parent);

public:
    // Used here, in HighlightingPage and in UserHighlightModel
    enum Column {
        Pattern = 0,
        FlashTaskbar = 1,
        PlaySound = 2,
        UseRegex = 3,
        CaseSensitive = 4,
        SoundPath = 5,
        Color = 6
    };

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

    friend class HighlightController;
};

}  // namespace chatterino
