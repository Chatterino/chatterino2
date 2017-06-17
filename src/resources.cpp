#include "resources.hpp"
#include "emotemanager.hpp"
#include "util/urlfetch.hpp"
#include "windowmanager.hpp"

#include <QPixmap>

namespace chatterino {

namespace {

inline messages::LazyLoadedImage *lli(EmoteManager &emoteManager, WindowManager &windowManager,
                                      const char *pixmapPath, qreal scale = 1)
{
    return new messages::LazyLoadedImage(emoteManager, windowManager, new QPixmap(pixmapPath),
                                         scale);
}

}  // namespace

Resources::Resources(EmoteManager &em, WindowManager &wm)
    : emoteManager(em)
    , windowManager(wm)
    , badgeStaff(lli(em, wm, ":/images/staff_bg.png"))
    , badgeAdmin(lli(em, wm, ":/images/admin_bg.png"))
    , badgeGlobalModerator(lli(em, wm, ":/images/globalmod_bg.png"))
    , badgeModerator(lli(em, wm, ":/images/moderator_bg.png"))
    , badgeTurbo(lli(em, wm, ":/images/turbo_bg.png"))
    , badgeBroadcaster(lli(em, wm, ":/images/broadcaster_bg.png"))
    , badgePremium(lli(em, wm, ":/images/twitchprime_bg.png"))
    , badgeVerified(lli(em, wm, ":/images/verified.png", 0.25))
    , cheerBadge100000(lli(em, wm, ":/images/cheer100000"))
    , cheerBadge10000(lli(em, wm, ":/images/cheer10000"))
    , cheerBadge5000(lli(em, wm, ":/images/cheer5000"))
    , cheerBadge1000(lli(em, wm, ":/images/cheer1000"))
    , cheerBadge100(lli(em, wm, ":/images/cheer100"))
    , cheerBadge1(lli(em, wm, ":/images/cheer1"))
    , buttonBan(lli(em, wm, ":/images/button_ban.png", 0.25))
    , buttonTimeout(lli(em, wm, ":/images/button_timeout.png", 0.25))
{
    QString badgesUrl("https://badges.twitch.tv/v1/badges/global/display?language=en");

    util::urlJsonFetch(badgesUrl, [this](QJsonObject &root) {
        QJsonObject sets = root.value("badge_sets").toObject();

        for (QJsonObject::iterator it = sets.begin(); it != sets.end(); ++it) {
            QJsonObject versions = it.value().toObject().value("versions").toObject();

            auto &badgeSet = this->badgeSets[it.key().toStdString()];
            auto &versionsMap = badgeSet.versions;

            for (auto versionIt = std::begin(versions); versionIt != std::end(versions);
                 ++versionIt) {
                std::string kkey = versionIt.key().toStdString();
                QJsonObject versionObj = versionIt.value().toObject();
                BadgeVersion v(std::move(versionObj), this->emoteManager, this->windowManager);
                versionsMap.emplace(kkey, v);
            }
        }

        this->dynamicBadgesLoaded = true;
    });
}

Resources::BadgeVersion::BadgeVersion(QJsonObject &&root, EmoteManager &emoteManager,
                                      WindowManager &windowManager)
    : badgeImage1x(new messages::LazyLoadedImage(emoteManager, windowManager,
                                                 root.value("image_url_1x").toString()))
    , badgeImage2x(new messages::LazyLoadedImage(emoteManager, windowManager,
                                                 root.value("image_url_2x").toString()))
    , badgeImage4x(new messages::LazyLoadedImage(emoteManager, windowManager,
                                                 root.value("image_url_4x").toString()))
    , description(root.value("description").toString().toStdString())
    , title(root.value("title").toString().toStdString())
    , clickAction(root.value("clickAction").toString().toStdString())
    , clickURL(root.value("clickURL").toString().toStdString())
{
}

void Resources::loadChannelData(const std::string &roomID, bool bypassCache)
{
    qDebug() << "Load channel data for" << QString::fromStdString(roomID);

    // Step 1: Get

    QString url = "https://badges.twitch.tv/v1/badges/channels/" + QString::fromStdString(roomID) +
                  "/display?language=en";

    util::urlJsonFetch(url, [this](QJsonObject &root) {
        QJsonObject sets = root.value("badge_sets").toObject();

        for (QJsonObject::iterator it = sets.begin(); it != sets.end(); ++it) {
            QJsonObject versions = it.value().toObject().value("versions").toObject();

            auto &badgeSet = this->badgeSets[it.key().toStdString()];
            auto &versionsMap = badgeSet.versions;

            for (auto versionIt = std::begin(versions); versionIt != std::end(versions);
                 ++versionIt) {
                std::string kkey = versionIt.key().toStdString();
                QJsonObject versionObj = versionIt.value().toObject();
                BadgeVersion v(std::move(versionObj), this->emoteManager, this->windowManager);
                versionsMap.emplace(kkey, v);
            }
        }

        this->dynamicBadgesLoaded = true;
    });
}

}  // namespace chatterino
