#pragma once

#include "common/Emotemap.hpp"

#include <QIcon>
#include <QRegularExpression>

#include <map>
#include <memory>
#include <mutex>

namespace chatterino {

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

    struct {
        QPixmap ban;
        QPixmap unban;
        QPixmap mod;
        QPixmap unmod;
    } buttons;

    chatterino::Image *badgeStaff;
    chatterino::Image *badgeAdmin;
    chatterino::Image *badgeGlobalModerator;
    chatterino::Image *badgeModerator;
    chatterino::Image *badgeTurbo;
    chatterino::Image *badgeBroadcaster;
    chatterino::Image *badgePremium;
    chatterino::Image *badgeVerified;
    chatterino::Image *badgeSubscriber;
    chatterino::Image *badgeCollapsed;

    chatterino::Image *cheerBadge100000;
    chatterino::Image *cheerBadge10000;
    chatterino::Image *cheerBadge5000;
    chatterino::Image *cheerBadge1000;
    chatterino::Image *cheerBadge100;
    chatterino::Image *cheerBadge1;

    chatterino::Image *moderationmode_enabled;
    chatterino::Image *moderationmode_disabled;

    chatterino::Image *splitHeaderContext;

    std::map<std::string, chatterino::Image *> cheerBadges;

    struct BadgeVersion {
        BadgeVersion() = delete;

        explicit BadgeVersion(QJsonObject &&root);

        chatterino::Image *badgeImage1x;
        chatterino::Image *badgeImage2x;
        chatterino::Image *badgeImage4x;
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

    chatterino::Image *buttonBan;
    chatterino::Image *buttonTimeout;

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
            std::map<QString, std::map<QString, std::map<QString, chatterino::Image *>>> images;
        };

        std::vector<CheermoteTier> tiers;
    };

    struct Cheermote {
        // a Cheermote indicates one tier
        QColor color;
        int minBits;

        EmoteData emoteDataAnimated;
        EmoteData emoteDataStatic;
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
        ChatterinoBadge(const std::string &_tooltip, chatterino::Image *_image)
            : tooltip(_tooltip)
            , image(_image)
        {
        }

        std::string tooltip;
        chatterino::Image *image;
    };

    //       username
    std::map<std::string, std::shared_ptr<ChatterinoBadge>> chatterinoBadges;

    void loadChannelData(const QString &roomID, bool bypassCache = false);
    void loadDynamicTwitchBadges();
    void loadChatterinoBadges();
};

}  // namespace chatterino
