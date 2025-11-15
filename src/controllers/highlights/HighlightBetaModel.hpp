#pragma once

#include "common/SignalVectorModel.hpp"

#include <qnamespace.h>
#include <QObject>

namespace chatterino {

struct SharedHighlight;

class HighlightBetaModel : public SignalVectorModel<SharedHighlight>
{
public:
    static constexpr int DATA_ROLE = Qt::UserRole + 110;

    explicit HighlightBetaModel(QObject *parent);

    // Used here, in HighlightingPage and in UserHighlightModel
    enum Column {
        Type = 0,
        Enabled = 1,
        Name = 2,
        Sound = 3,
        COUNT,
    };

protected:
    // turn a vector item into a model row
    SharedHighlight getItemFromRow(std::vector<QStandardItem *> &row,
                                   const SharedHighlight &original) override;

    // turns a row in the model into a vector item
    void getRowFromItem(const SharedHighlight &item,
                        std::vector<QStandardItem *> &row) override;

    void afterInit() override;

    void customRowSetData(const std::vector<QStandardItem *> &row, int column,
                          const QVariant &value, int role,
                          int rowIndex) override;
};

}  // namespace chatterino
