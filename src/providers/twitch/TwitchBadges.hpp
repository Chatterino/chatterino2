#pragma once

#include <QMap>
#include <QString>
#include <boost/optional.hpp>
#include <unordered_map>

#include "common/UniqueAccess.hpp"
#include "messages/Image.hpp"
#include "util/DisplayBadge.hpp"
#include "util/QStringHash.hpp"

#include "pajlada/signals/signal.hpp"

#include <memory>
#include <queue>
#include <shared_mutex>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class Settings;
class Paths;

class TwitchBadges
{
    using QIconPtr = std::shared_ptr<QIcon>;
    using ImagePtr = std::shared_ptr<Image>;
    using BadgeIconCallback = std::function<void(QString, const QIconPtr)>;

public:
    static TwitchBadges *instance();

    // Get badge from name and version
    boost::optional<EmotePtr> badge(const QString &set,
                                    const QString &version) const;
    // Get first matching badge with name, regardless of version
    boost::optional<EmotePtr> badge(const QString &set) const;

    void getBadgeIcon(const QString &name, BadgeIconCallback callback);
    void getBadgeIcon(const DisplayBadge &badge, BadgeIconCallback callback);
    void getBadgeIcons(const QList<DisplayBadge> &badges,
                       BadgeIconCallback callback);

private:
    static TwitchBadges *instance_;

    TwitchBadges();
    void loadTwitchBadges();
    void loaded();
    void loadEmoteImage(const QString &name, ImagePtr image,
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
