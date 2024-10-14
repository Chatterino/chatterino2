#pragma once

#include "common/UniqueAccess.hpp"
#include "util/QStringHash.hpp"

#include <pajlada/signals/signal.hpp>
#include <QIcon>
#include <QJsonObject>
#include <QMap>
#include <QString>

#include <memory>
#include <optional>
#include <queue>
#include <shared_mutex>
#include <unordered_map>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class Settings;
class Paths;
class Image;
class DisplayBadge;

class TwitchBadges
{
    using QIconPtr = std::shared_ptr<QIcon>;
    using ImagePtr = std::shared_ptr<Image>;
    using BadgeIconCallback = std::function<void(QString, const QIconPtr)>;

public:
    // Get badge from name and version
    std::optional<EmotePtr> badge(const QString &set,
                                  const QString &version) const;
    // Get first matching badge with name, regardless of version
    std::optional<EmotePtr> badge(const QString &set) const;

    void getBadgeIcon(const QString &name, BadgeIconCallback callback);
    void getBadgeIcon(const DisplayBadge &badge, BadgeIconCallback callback);
    void getBadgeIcons(const QList<DisplayBadge> &badges,
                       BadgeIconCallback callback);

    void loadTwitchBadges();

    /// Loads the badges shipped with Chatterino (twitch-badges.json)
    void loadLocalBadges();

private:
    void loaded();
    void loadEmoteImage(const QString &name, const ImagePtr &image,
                        BadgeIconCallback &&callback);

    std::shared_mutex badgesMutex_;
    QMap<QString, QIconPtr> badgesMap_;

    std::mutex queueMutex_;
    std::queue<QPair<QString, BadgeIconCallback>> callbackQueue_;

    std::shared_mutex loadedMutex_;
    bool loaded_ = false;

    UniqueAccess<
        std::unordered_map<QString, std::unordered_map<QString, EmotePtr>>>
        badgeSets_;  // "bits": { "100": ... "500": ...
};

}  // namespace chatterino
