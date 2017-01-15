#include "resources.h"
#include "QPixmap"

LazyLoadedImage *Resources::m_badgeStaff;
LazyLoadedImage *Resources::m_badgeAdmin;
LazyLoadedImage *Resources::m_badgeModerator;
LazyLoadedImage *Resources::m_badgeGlobalmod;
LazyLoadedImage *Resources::m_badgeTurbo;
LazyLoadedImage *Resources::m_badgeBroadcaster;
LazyLoadedImage *Resources::m_badgePremium;

LazyLoadedImage *Resources::m_cheerBadge100000;
LazyLoadedImage *Resources::m_cheerBadge10000;
LazyLoadedImage *Resources::m_cheerBadge5000;
LazyLoadedImage *Resources::m_cheerBadge1000;
LazyLoadedImage *Resources::m_cheerBadge100;
LazyLoadedImage *Resources::m_cheerBadge1;

LazyLoadedImage *Resources::m_buttonBan;
LazyLoadedImage *Resources::m_buttonTimeout;

Resources::Resources()
{
}

void
Resources::load()
{
    // badges
    m_badgeStaff = new LazyLoadedImage(new QPixmap(":/images/staff_bg.png"));
    m_badgeAdmin = new LazyLoadedImage(new QPixmap(":/images/admin_bg.png"));
    m_badgeModerator =
        new LazyLoadedImage(new QPixmap(":/images/moderator_bg.png"));
    m_badgeGlobalmod =
        new LazyLoadedImage(new QPixmap(":/images/globalmod_bg.png"));
    m_badgeTurbo = new LazyLoadedImage(new QPixmap(":/images/turbo_bg.png"));
    m_badgeBroadcaster =
        new LazyLoadedImage(new QPixmap(":/images/broadcaster_bg.png"));
    m_badgePremium =
        new LazyLoadedImage(new QPixmap(":/images/twitchprime_bg.png"));

    // cheer badges
    m_cheerBadge100000 =
        new LazyLoadedImage(new QPixmap(":/images/cheer100000"));
    m_cheerBadge10000 = new LazyLoadedImage(new QPixmap(":/images/cheer10000"));
    m_cheerBadge5000 = new LazyLoadedImage(new QPixmap(":/images/cheer5000"));
    m_cheerBadge1000 = new LazyLoadedImage(new QPixmap(":/images/cheer1000"));
    m_cheerBadge100 = new LazyLoadedImage(new QPixmap(":/images/cheer100"));
    m_cheerBadge1 = new LazyLoadedImage(new QPixmap(":/images/cheer1"));

    // button
    m_buttonBan =
        new LazyLoadedImage(new QPixmap(":/images/button_ban.png"), 0.25);
    m_buttonTimeout =
        new LazyLoadedImage(new QPixmap(":/images/button_timeout.png"), 0.25);
}
