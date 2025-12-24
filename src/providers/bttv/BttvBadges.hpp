#pragma once

#include "util/BadgeRegistry.hpp"

#include <QJsonObject>

#include <memory>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class BttvBadges : public BadgeRegistry
{
public:
    BttvBadges() = default;

protected:
    QString idForBadge(const QJsonObject &badgeJson) const override;
    EmotePtr createBadge(const QString &id,
                         const QJsonObject &badgeJson) const override;
};

}  // namespace chatterino
