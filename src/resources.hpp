#pragma once

#include "messages/lazyloadedimage.hpp"

namespace chatterino {

class Resources
{
public:
    static void load();

    // badges
    static messages::LazyLoadedImage *getBadgeStaff()
    {
        return badgeStaff;
    }

    static messages::LazyLoadedImage *getBadgeAdmin()
    {
        return badgeAdmin;
    }

    static messages::LazyLoadedImage *getBadgeGlobalmod()
    {
        return badgeGlobalmod;
    }

    static messages::LazyLoadedImage *getBadgeModerator()
    {
        return badgeModerator;
    }

    static messages::LazyLoadedImage *getBadgeTurbo()
    {
        return badgeTurbo;
    }

    static messages::LazyLoadedImage *getBadgeBroadcaster()
    {
        return badgeBroadcaster;
    }

    static messages::LazyLoadedImage *getBadgePremium()
    {
        return badgePremium;
    }

    // cheer badges
    static messages::LazyLoadedImage *getCheerBadge100000()
    {
        return cheerBadge100000;
    }

    static messages::LazyLoadedImage *getCheerBadge10000()
    {
        return cheerBadge10000;
    }

    static messages::LazyLoadedImage *getCheerBadge5000()
    {
        return cheerBadge5000;
    }

    static messages::LazyLoadedImage *getCheerBadge1000()
    {
        return cheerBadge1000;
    }

    static messages::LazyLoadedImage *getCheerBadge100()
    {
        return cheerBadge100;
    }

    static messages::LazyLoadedImage *getCheerBadge1()
    {
        return cheerBadge1;
    }

    static messages::LazyLoadedImage *getButtonBan()
    {
        return buttonBan;
    }

    static messages::LazyLoadedImage *getButtonTimeout()
    {
        return buttonTimeout;
    }

private:
    Resources();

    static messages::LazyLoadedImage *badgeStaff;
    static messages::LazyLoadedImage *badgeAdmin;
    static messages::LazyLoadedImage *badgeGlobalmod;
    static messages::LazyLoadedImage *badgeModerator;
    static messages::LazyLoadedImage *badgeTurbo;
    static messages::LazyLoadedImage *badgeBroadcaster;
    static messages::LazyLoadedImage *badgePremium;

    static messages::LazyLoadedImage *cheerBadge100000;
    static messages::LazyLoadedImage *cheerBadge10000;
    static messages::LazyLoadedImage *cheerBadge5000;
    static messages::LazyLoadedImage *cheerBadge1000;
    static messages::LazyLoadedImage *cheerBadge100;
    static messages::LazyLoadedImage *cheerBadge1;

    static messages::LazyLoadedImage *buttonBan;
    static messages::LazyLoadedImage *buttonTimeout;
};

}  // namespace chatterino
