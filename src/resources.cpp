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

Resources::Resources(EmoteManager &emoteManager, WindowManager &windowManager)
    : badgeStaff(lli(emoteManager, windowManager, ":/images/staff_bg.png"))
    , badgeAdmin(lli(emoteManager, windowManager, ":/images/admin_bg.png"))
    , badgeGlobalModerator(lli(emoteManager, windowManager, ":/images/globalmod_bg.png"))
    , badgeModerator(lli(emoteManager, windowManager, ":/images/moderator_bg.png"))
    , badgeTurbo(lli(emoteManager, windowManager, ":/images/turbo_bg.png"))
    , badgeBroadcaster(lli(emoteManager, windowManager, ":/images/broadcaster_bg.png"))
    , badgePremium(lli(emoteManager, windowManager, ":/images/twitchprime_bg.png"))
    , badgeVerified(lli(emoteManager, windowManager, ":/images/verified.png", 0.25))
    , cheerBadge100000(lli(emoteManager, windowManager, ":/images/cheer100000"))
    , cheerBadge10000(lli(emoteManager, windowManager, ":/images/cheer10000"))
    , cheerBadge5000(lli(emoteManager, windowManager, ":/images/cheer5000"))
    , cheerBadge1000(lli(emoteManager, windowManager, ":/images/cheer1000"))
    , cheerBadge100(lli(emoteManager, windowManager, ":/images/cheer100"))
    , cheerBadge1(lli(emoteManager, windowManager, ":/images/cheer1"))
    , buttonBan(lli(emoteManager, windowManager, ":/images/button_ban.png", 0.25))
    , buttonTimeout(lli(emoteManager, windowManager, ":/images/button_timeout.png", 0.25))
{
    QString badgesUrl("https://badges.twitch.tv/v1/badges/global/display?language=en");

    util::urlJsonFetch(badgesUrl, [this](QJsonObject &root) {
        QJsonObject sets = root.value("badge_sets").toObject();

        for (auto it = std::begin(sets); it != std::end(sets); ++it) {
            printf("%s\n", qPrintable(it.key()));

            auto &badgeSet = this->badgeSets[it.key().toStdString()];

            std::map<std::string, BadgeVersion> &versionsMap = badgeSet.versions;

            QJsonObject versions = sets.value("versions").toObject();

            for (auto versionIt = std::begin(versions); versionIt != std::end(versions);
                 ++versionIt) {
            /*
                std::string kkey = versionIt.key().toStdString();
                QJsonObject versionObj = versionIt.value().toObject();
                versionsMap.emplace(std::make_pair(kkey, versionObj));
                */
            }
        }
    });
}

Resources::BadgeVersion::BadgeVersion(QJsonObject &&root)
{
}

void Resources::Channel::loadData()
{
    /*
    if (this->loaded) {
        return;
    }

    this->loaded = true;

    if (this->id.empty()) {
        //util::urlJsonFetch()
    }
    */
}

}  // namespace chatterino
