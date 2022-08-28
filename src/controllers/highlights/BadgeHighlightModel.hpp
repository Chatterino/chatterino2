#pragma once

#include <QObject>

#include "common/SignalVectorModel.hpp"
#include "controllers/highlights/HighlightBadge.hpp"
#include "providers/twitch/TwitchBadges.hpp"

namespace chatterino {

class HighlightController;

class BadgeHighlightModel : public SignalVectorModel<HighlightBadge>
{
public:
    explicit BadgeHighlightModel(QObject *parent);

    enum Column {
        Badge = 0,
        FlashTaskbar = 1,
        PlaySound = 2,
        SoundPath = 3,
        Color = 4
    };

protected:
    // vector into model row
    virtual HighlightBadge getItemFromRow(
        std::vector<QStandardItem *> &row,
        const HighlightBadge &original) override;

    virtual void getRowFromItem(const HighlightBadge &item,
                                std::vector<QStandardItem *> &row) override;
};

}  // namespace chatterino
