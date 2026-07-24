#pragma once

#include "common/SignalVectorModel.hpp"
#include "controllers/highlights/types/AllForward.hpp"

#include <QObject>
#include <QStandardItemModel>

#include <vector>

namespace chatterino::highlights {

struct SharedHighlight;

class Model : public SignalVectorModel<AllHighlights>
{
public:
    static constexpr int DATA_ROLE = Qt::UserRole + 110;
    static constexpr int ID_ROLE = Qt::UserRole + 111;

    explicit Model(QObject *parent);

    // Used here, in HighlightingPage and in UserHighlightModel
    enum Column {
        Enabled = 0,
        Sound = 1,
        Name = 2,
        COUNT,
    };

protected:
    // turn a vector item into a model row
    AllHighlights getItemFromRow(std::vector<QStandardItem *> &row,
                                 const AllHighlights &original) override;

    // turns a row in the model into a vector item
    void getRowFromItem(const AllHighlights &item,
                        std::vector<QStandardItem *> &row) override;
};

}  // namespace chatterino::highlights
