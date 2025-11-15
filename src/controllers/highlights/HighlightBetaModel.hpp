#pragma once

#include "common/SignalVectorModel.hpp"

#include <qnamespace.h>
#include <QObject>

namespace chatterino {

class HighlightPhrase;

class HighlightBetaModel : public SignalVectorModel<HighlightPhrase>
{
public:
    static constexpr int DATA_ROLE = Qt::UserRole + 110;

    explicit HighlightBetaModel(QObject *parent);

    // Used here, in HighlightingPage and in UserHighlightModel
    enum Column {
        Enabled = 0,
        Type = 1,
        Name = 2,
        Color = 3,
        Sound = 4,
        Configure = 5,
        COUNT,
        Data = 100,
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
