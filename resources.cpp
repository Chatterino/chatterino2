#include "resources.h"

#include <QPixmap>

namespace chatterino {

messages::LazyLoadedImage *Resources::badgeStaff(NULL);
messages::LazyLoadedImage *Resources::badgeAdmin(NULL);
messages::LazyLoadedImage *Resources::badgeModerator(NULL);
messages::LazyLoadedImage *Resources::badgeGlobalmod(NULL);
messages::LazyLoadedImage *Resources::badgeTurbo(NULL);
messages::LazyLoadedImage *Resources::badgeBroadcaster(NULL);
messages::LazyLoadedImage *Resources::badgePremium(NULL);

messages::LazyLoadedImage *Resources::cheerBadge100000(NULL);
messages::LazyLoadedImage *Resources::cheerBadge10000(NULL);
messages::LazyLoadedImage *Resources::cheerBadge5000(NULL);
messages::LazyLoadedImage *Resources::cheerBadge1000(NULL);
messages::LazyLoadedImage *Resources::cheerBadge100(NULL);
messages::LazyLoadedImage *Resources::cheerBadge1(NULL);

messages::LazyLoadedImage *Resources::buttonBan(NULL);
messages::LazyLoadedImage *Resources::buttonTimeout(NULL);

Resources::Resources()
{
}

void Resources::load()
{
    // badges
    Resources::badgeStaff = new messages::LazyLoadedImage(new QPixmap(":/images/staff_bg.png"));
    Resources::badgeAdmin = new messages::LazyLoadedImage(new QPixmap(":/images/admin_bg.png"));
    Resources::badgeModerator =
        new messages::LazyLoadedImage(new QPixmap(":/images/moderator_bg.png"));
    Resources::badgeGlobalmod =
        new messages::LazyLoadedImage(new QPixmap(":/images/globalmod_bg.png"));
    Resources::badgeTurbo = new messages::LazyLoadedImage(new QPixmap(":/images/turbo_bg.png"));
    Resources::badgeBroadcaster =
        new messages::LazyLoadedImage(new QPixmap(":/images/broadcaster_bg.png"));
    Resources::badgePremium =
        new messages::LazyLoadedImage(new QPixmap(":/images/twitchprime_bg.png"));

    // cheer badges
    Resources::cheerBadge100000 =
        new messages::LazyLoadedImage(new QPixmap(":/images/cheer100000"));
    Resources::cheerBadge10000 = new messages::LazyLoadedImage(new QPixmap(":/images/cheer10000"));
    Resources::cheerBadge5000 = new messages::LazyLoadedImage(new QPixmap(":/images/cheer5000"));
    Resources::cheerBadge1000 = new messages::LazyLoadedImage(new QPixmap(":/images/cheer1000"));
    Resources::cheerBadge100 = new messages::LazyLoadedImage(new QPixmap(":/images/cheer100"));
    Resources::cheerBadge1 = new messages::LazyLoadedImage(new QPixmap(":/images/cheer1"));

    // button
    Resources::buttonBan =
        new messages::LazyLoadedImage(new QPixmap(":/images/button_ban.png"), 0.25);
    Resources::buttonTimeout =
        new messages::LazyLoadedImage(new QPixmap(":/images/button_timeout.png"), 0.25);
}
}
