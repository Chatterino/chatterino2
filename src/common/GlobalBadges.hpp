#pragma once

#include "messages/Image.hpp"
#include "providers/twitch/TwitchBadges.hpp"
#include "util/DisplayBadge.hpp"

#include <QIcon>
#include <QMap>

namespace chatterino {

class GlobalBadges
{
    using QIconPtr = std::shared_ptr<QIcon>;
    using ImagePtr = std::shared_ptr<Image>;
    using BadgeIconCallback = std::function<void(QString, const QIconPtr)>;

public:
    static GlobalBadges *instance();
    void update();

    void getBadgeIcon(const QString &identifier, BadgeIconCallback callback);
    void getBadgeIcon(const DisplayBadge &badge, BadgeIconCallback callback);
    void getBadgeIcons(const QList<DisplayBadge> &badges,
                       BadgeIconCallback callback);

private:
    static GlobalBadges *instance_;

    GlobalBadges();
    void loadEmoteImage(const QString &identifier, ImagePtr image,
                        BadgeIconCallback &&callback);

    TwitchBadges *badges_;
    QMap<QString, QIconPtr> badgesMap_;
    std::queue<QPair<QString, BadgeIconCallback>> callbackQueue_;

    bool loading_ = false;

    std::mutex loadingMutex_;
    std::mutex queueMutex_;
    std::mutex mapMutex_;
};

}  // namespace chatterino
