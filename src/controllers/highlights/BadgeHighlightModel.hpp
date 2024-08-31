#pragma once

#include "common/SignalVectorModel.hpp"

#include <QObject>

namespace chatterino {

class HighlightController;
class HighlightBadge;

class BadgeHighlightModel : public SignalVectorModel<HighlightBadge>
{
public:
    explicit BadgeHighlightModel(QObject *parent);

    enum Column {
        Badge = 0,
        ShowInMentions = 1,
        FlashTaskbar = 2,
        PlaySound = 3,
        SoundPath = 4,
        Color = 5
    };

protected:
    // vector into model row
    HighlightBadge getItemFromRow(std::vector<QStandardItem *> &row,
                                  const HighlightBadge &original) override;

    void getRowFromItem(const HighlightBadge &item,
                        std::vector<QStandardItem *> &row) override;
};

}  // namespace chatterino
