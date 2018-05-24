#pragma once

#include "util/emotemap.hpp"

#include <QIcon>
#include <QRegularExpression>

#include <map>
#include <memory>
#include <mutex>

namespace chatterino {
namespace singletons {

class ResourceManager
{
public:
    ResourceManager();

    ~ResourceManager() = delete;

    void initialize();

    struct {
        QIcon left;
        QIcon right;
        QIcon up;
        QIcon down;
        QIcon move;
    } split;

    messages::Image *badgeStaff;
    messages::Image *badgeAdmin;
    messages::Image *badgeGlobalModerator;
    messages::Image *badgeModerator;
    messages::Image *badgeTurbo;
    messages::Image *badgeBroadcaster;
    messages::Image *badgePremium;
    messages::Image *badgeVerified;
    messages::Image *badgeSubscriber;
    messages::Image *badgeCollapsed;

    messages::Image *cheerBadge100000;
    messages::Image *cheerBadge10000;
    messages::Image *cheerBadge5000;
    messages::Image *cheerBadge1000;
    messages::Image *cheerBadge100;
    messages::Image *cheerBadge1;

    messages::Image *moderationmode_enabled;
    messages::Image *moderationmode_disabled;

    messages::Image *splitHeaderContext;

    std::map<std::string, messages::Image *> cheerBadges;

    struct BadgeVersion {
        BadgeVersion() = delete;

        explicit BadgeVersion(QJsonObject &&root);

        messages::Image *badgeImage1x;
        messages::Image *badgeImage2x;
        messages::Image *badgeImage4x;
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

    messages::Image *buttonBan;
    messages::Image *buttonTimeout;

    struct JSONCheermoteSet {
        QString prefix;
        std::vector<QString> scales;

        std::vector<QString> backgrounds;
        std::vector<QString> states;

        QString type;
        QString updatedAt;
        int priority;

        struct CheermoteTier {
            int minBits;
            QString id;
            QString color;

            //       Background        State             Scale
            std::map<QString, std::map<QString, std::map<QString, messages::Image *>>> images;
        };

        std::vector<CheermoteTier> tiers;
    };

    struct Cheermote {
        // a Cheermote indicates one tier
        QColor color;
        int minBits;

        util::EmoteData emoteDataAnimated;
        util::EmoteData emoteDataStatic;
    };

    struct CheermoteSet {
        QRegularExpression regex;
        std::vector<Cheermote> cheermotes;
    };

    struct Channel {
        std::map<std::string, BadgeSet> badgeSets;
        std::vector<JSONCheermoteSet> jsonCheermoteSets;
        std::vector<CheermoteSet> cheermoteSets;

        bool loaded = false;
    };

    //       channelId
    std::map<QString, Channel> channels;

    // Chatterino badges
    struct ChatterinoBadge {
        ChatterinoBadge(const std::string &_tooltip, messages::Image *_image)
            : tooltip(_tooltip)
            , image(_image)
        {
        }

        std::string tooltip;
        messages::Image *image;
    };

    //       username
    std::map<std::string, std::shared_ptr<ChatterinoBadge>> chatterinoBadges;

    void loadChannelData(const QString &roomID, bool bypassCache = false);
    void loadDynamicTwitchBadges();
    void loadChatterinoBadges();
};

}  // namespace singletons
}  // namespace chatterino
