#include "resources.h"

#include <QPixmap>

LazyLoadedImage *Resources::badgeStaff(NULL);
LazyLoadedImage *Resources::badgeAdmin(NULL);
LazyLoadedImage *Resources::badgeModerator(NULL);
LazyLoadedImage *Resources::badgeGlobalmod(NULL);
LazyLoadedImage *Resources::badgeTurbo(NULL);
LazyLoadedImage *Resources::badgeBroadcaster(NULL);
LazyLoadedImage *Resources::badgePremium(NULL);

LazyLoadedImage *Resources::cheerBadge100000(NULL);
LazyLoadedImage *Resources::cheerBadge10000(NULL);
LazyLoadedImage *Resources::cheerBadge5000(NULL);
LazyLoadedImage *Resources::cheerBadge1000(NULL);
LazyLoadedImage *Resources::cheerBadge100(NULL);
LazyLoadedImage *Resources::cheerBadge1(NULL);

LazyLoadedImage *Resources::buttonBan(NULL);
LazyLoadedImage *Resources::buttonTimeout(NULL);

Resources::Resources()
{
}

void
Resources::load()
{
    // badges
    Resources::badgeStaff =
        new LazyLoadedImage(new QPixmap(":/images/staff_bg.png"));
    Resources::badgeAdmin =
        new LazyLoadedImage(new QPixmap(":/images/admin_bg.png"));
    Resources::badgeModerator =
        new LazyLoadedImage(new QPixmap(":/images/moderator_bg.png"));
    Resources::badgeGlobalmod =
        new LazyLoadedImage(new QPixmap(":/images/globalmod_bg.png"));
    Resources::badgeTurbo =
        new LazyLoadedImage(new QPixmap(":/images/turbo_bg.png"));
    Resources::badgeBroadcaster =
        new LazyLoadedImage(new QPixmap(":/images/broadcaster_bg.png"));
    Resources::badgePremium =
        new LazyLoadedImage(new QPixmap(":/images/twitchprime_bg.png"));

    // cheer badges
    Resources::cheerBadge100000 =
        new LazyLoadedImage(new QPixmap(":/images/cheer100000"));
    Resources::cheerBadge10000 =
        new LazyLoadedImage(new QPixmap(":/images/cheer10000"));
    Resources::cheerBadge5000 =
        new LazyLoadedImage(new QPixmap(":/images/cheer5000"));
    Resources::cheerBadge1000 =
        new LazyLoadedImage(new QPixmap(":/images/cheer1000"));
    Resources::cheerBadge100 =
        new LazyLoadedImage(new QPixmap(":/images/cheer100"));
    Resources::cheerBadge1 =
        new LazyLoadedImage(new QPixmap(":/images/cheer1"));

    // button
    Resources::buttonBan =
        new LazyLoadedImage(new QPixmap(":/images/button_ban.png"), 0.25);
    Resources::buttonTimeout =
        new LazyLoadedImage(new QPixmap(":/images/button_timeout.png"), 0.25);
}
