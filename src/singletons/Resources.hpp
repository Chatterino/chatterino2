#pragma once

#include "common/Singleton.hpp"

#include "common/Emotemap.hpp"

#include <QIcon>
#include <QRegularExpression>

#include <map>
#include <memory>
#include <mutex>

namespace chatterino {

class Resources : public Singleton
{
public:
    Resources();

    ~Resources() = delete;

    virtual void initialize(Application &app) override;

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

    Image *badgeStaff;
    Image *badgeAdmin;
    Image *badgeGlobalModerator;
    Image *badgeModerator;
    Image *badgeTurbo;
    Image *badgeBroadcaster;
    Image *badgePremium;
    Image *badgeVerified;
    Image *badgeSubscriber;
    Image *badgeCollapsed;

    Image *cheerBadge100000;
    Image *cheerBadge10000;
    Image *cheerBadge5000;
    Image *cheerBadge1000;
    Image *cheerBadge100;
    Image *cheerBadge1;

    Image *moderationmode_enabled;
    Image *moderationmode_disabled;

    Image *splitHeaderContext;

    std::map<std::string, Image *> cheerBadges;

    struct BadgeVersion {
        BadgeVersion() = delete;

        explicit BadgeVersion(QJsonObject &&root);

        Image *badgeImage1x;
        Image *badgeImage2x;
        Image *badgeImage4x;
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

    Image *buttonBan;
    Image *buttonTimeout;

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
            std::map<QString, std::map<QString, std::map<QString, Image *>>> images;
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
        ChatterinoBadge(const std::string &_tooltip, Image *_image)
            : tooltip(_tooltip)
            , image(_image)
        {
        }

        std::string tooltip;
        Image *image;
    };

    //       username
    std::map<std::string, std::shared_ptr<ChatterinoBadge>> chatterinoBadges;

    void loadChannelData(const QString &roomID, bool bypassCache = false);
    void loadDynamicTwitchBadges();
    void loadChatterinoBadges();
};

}  // namespace chatterino
