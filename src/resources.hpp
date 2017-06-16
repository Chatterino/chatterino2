#pragma once

#include "messages/lazyloadedimage.hpp"

#include <map>
#include <mutex>

namespace chatterino {

class EmoteManager;
class WindowManager;

class Resources
{
public:
    explicit Resources(EmoteManager &emoteManager, WindowManager &windowManager);

    messages::LazyLoadedImage *badgeStaff;
    messages::LazyLoadedImage *badgeAdmin;
    messages::LazyLoadedImage *badgeGlobalModerator;
    messages::LazyLoadedImage *badgeModerator;
    messages::LazyLoadedImage *badgeTurbo;
    messages::LazyLoadedImage *badgeBroadcaster;
    messages::LazyLoadedImage *badgePremium;
    messages::LazyLoadedImage *badgeVerified;

    messages::LazyLoadedImage *cheerBadge100000;
    messages::LazyLoadedImage *cheerBadge10000;
    messages::LazyLoadedImage *cheerBadge5000;
    messages::LazyLoadedImage *cheerBadge1000;
    messages::LazyLoadedImage *cheerBadge100;
    messages::LazyLoadedImage *cheerBadge1;

    std::map<std::string, messages::LazyLoadedImage *> cheerBadges;

    struct BadgeVersion {
        BadgeVersion() = delete;

        explicit BadgeVersion(QJsonObject &&root, EmoteManager &emoteManager,
                              WindowManager &windowManager);

        messages::LazyLoadedImage *badgeImage1x;
        messages::LazyLoadedImage *badgeImage2x;
        messages::LazyLoadedImage *badgeImage4x;
        std::string description;
        std::string title;
        std::string clickAction;
        std::string clickURL;
    };

    struct BadgeSet {
        std::map<std::string, BadgeVersion> versions;
    };

    std::map<std::string, BadgeSet> badgeSets;

    bool dynamicBadgesLoaded = false;

    messages::LazyLoadedImage *buttonBan;
    messages::LazyLoadedImage *buttonTimeout;

    struct Channel {
        std::string id;

        std::mutex globalMapMutex;

        void loadData();

        // std::atomic<bool> loaded = false;
    };

    //       channelId
    std::map<std::string, Channel> channels;

    void loadChannelData(const std::string &roomID, bool bypassCache = false);
};

}  // namespace chatterino
