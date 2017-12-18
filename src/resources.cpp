#include "resources.hpp"
#include "emotemanager.hpp"
#include "util/urlfetch.hpp"
#include "windowmanager.hpp"

#include <QPixmap>

namespace chatterino {

namespace {

inline messages::LazyLoadedImage *lli(const char *pixmapPath, qreal scale = 1)
{
    return new messages::LazyLoadedImage(new QPixmap(pixmapPath), scale);
}

}  // namespace

Resources::Resources()
    : badgeStaff(lli(":/images/staff_bg.png"))
    , badgeAdmin(lli(":/images/admin_bg.png"))
    , badgeGlobalModerator(lli(":/images/globalmod_bg.png"))
    , badgeModerator(lli(":/images/moderator_bg.png"))
    , badgeTurbo(lli(":/images/turbo_bg.png"))
    , badgeBroadcaster(lli(":/images/broadcaster_bg.png"))
    , badgePremium(lli(":/images/twitchprime_bg.png"))
    , badgeVerified(lli(":/images/verified.png", 0.25))
    , badgeSubscriber(lli(":/images/subscriber.png", 0.25))
    , badgeCollapsed(lli(":/images/collapse.png"))
    , cheerBadge100000(lli(":/images/cheer100000"))
    , cheerBadge10000(lli(":/images/cheer10000"))
    , cheerBadge5000(lli(":/images/cheer5000"))
    , cheerBadge1000(lli(":/images/cheer1000"))
    , cheerBadge100(lli(":/images/cheer100"))
    , cheerBadge1(lli(":/images/cheer1"))
    , buttonBan(lli(":/images/button_ban.png", 0.25))
    , buttonTimeout(lli(":/images/button_timeout.png", 0.25))
{
    this->loadDynamicTwitchBadges();

    this->loadChatterinoBadges();
}

Resources::BadgeVersion::BadgeVersion(QJsonObject &&root)
    : badgeImage1x(new messages::LazyLoadedImage(root.value("image_url_1x").toString()))
    , badgeImage2x(new messages::LazyLoadedImage(root.value("image_url_2x").toString()))
    , badgeImage4x(new messages::LazyLoadedImage(root.value("image_url_4x").toString()))
    , description(root.value("description").toString().toStdString())
    , title(root.value("title").toString().toStdString())
    , clickAction(root.value("clickAction").toString().toStdString())
    , clickURL(root.value("clickURL").toString().toStdString())
{
}

void Resources::loadChannelData(const QString &roomID, bool bypassCache)
{
    qDebug() << "Load channel data for" << roomID;

    QString url = "https://badges.twitch.tv/v1/badges/channels/" + roomID + "/display?language=en";

    util::NetworkRequest req(url);
    req.setCaller(QThread::currentThread());

    req.getJSON([this, roomID](QJsonObject &root) {
        QJsonObject sets = root.value("badge_sets").toObject();

        Resources::Channel &ch = this->channels[roomID];

        for (QJsonObject::iterator it = sets.begin(); it != sets.end(); ++it) {
            QJsonObject versions = it.value().toObject().value("versions").toObject();

            auto &badgeSet = ch.badgeSets[it.key().toStdString()];
            auto &versionsMap = badgeSet.versions;

            for (auto versionIt = std::begin(versions); versionIt != std::end(versions);
                 ++versionIt) {
                std::string kkey = versionIt.key().toStdString();
                QJsonObject versionObj = versionIt.value().toObject();
                BadgeVersion v(std::move(versionObj));
                versionsMap.emplace(kkey, v);
            }
        }

        ch.loaded = true;
    });
}

void Resources::loadDynamicTwitchBadges()
{
    static QString url("https://badges.twitch.tv/v1/badges/global/display?language=en");

    util::NetworkRequest req(url);
    req.setCaller(QThread::currentThread());
    req.getJSON([this](QJsonObject &root) {
        QJsonObject sets = root.value("badge_sets").toObject();
        qDebug() << "badges fetched";
        for (QJsonObject::iterator it = sets.begin(); it != sets.end(); ++it) {
            QJsonObject versions = it.value().toObject().value("versions").toObject();

            auto &badgeSet = this->badgeSets[it.key().toStdString()];
            auto &versionsMap = badgeSet.versions;

            for (auto versionIt = std::begin(versions); versionIt != std::end(versions);
                 ++versionIt) {
                std::string kkey = versionIt.key().toStdString();
                QJsonObject versionObj = versionIt.value().toObject();
                BadgeVersion v(std::move(versionObj));
                versionsMap.emplace(kkey, v);
            }
        }

        this->dynamicBadgesLoaded = true;
    });
}

void Resources::loadChatterinoBadges()
{
    this->chatterinoBadges.clear();

    static QString url("https://fourtf.com/chatterino/badges.json");

    util::NetworkRequest req(url);
    req.setCaller(QThread::currentThread());

    req.getJSON([this](QJsonObject &root) {
        QJsonArray badgeVariants = root.value("badges").toArray();
        qDebug() << "chatbadges fetched";
        for (QJsonArray::iterator it = badgeVariants.begin(); it != badgeVariants.end(); ++it) {
            QJsonObject badgeVariant = it->toObject();
            const std::string badgeVariantTooltip =
                badgeVariant.value("tooltip").toString().toStdString();
            const QString &badgeVariantImageURL = badgeVariant.value("image").toString();

            auto badgeVariantPtr = std::make_shared<ChatterinoBadge>(
                badgeVariantTooltip, new messages::LazyLoadedImage(badgeVariantImageURL));

            QJsonArray badgeVariantUsers = badgeVariant.value("users").toArray();

            for (QJsonArray::iterator it = badgeVariantUsers.begin(); it != badgeVariantUsers.end();
                 ++it) {
                const std::string username = it->toString().toStdString();
                this->chatterinoBadges[username] =
                    std::shared_ptr<ChatterinoBadge>(badgeVariantPtr);
            }
        }
    });
}

}  // namespace chatterino
