#ifndef RESOURCES_H
#define RESOURCES_H

#include <lazyloadedimage.h>

class Resources
{
public:
    static void load();

    // badges
    static LazyLoadedImage *
    getBadgeStaff()
    {
        return badgeStaff;
    }

    static LazyLoadedImage *
    getBadgeAdmin()
    {
        return badgeAdmin;
    }

    static LazyLoadedImage *
    getBadgeGlobalmod()
    {
        return badgeGlobalmod;
    }

    static LazyLoadedImage *
    getBadgeModerator()
    {
        return badgeModerator;
    }

    static LazyLoadedImage *
    getBadgeTurbo()
    {
        return badgeTurbo;
    }

    static LazyLoadedImage *
    getBadgeBroadcaster()
    {
        return badgeBroadcaster;
    }

    static LazyLoadedImage *
    getBadgePremium()
    {
        return badgePremium;
    }

    // cheer badges
    static LazyLoadedImage *
    getCheerBadge100000()
    {
        return cheerBadge100000;
    }

    static LazyLoadedImage *
    getCheerBadge10000()
    {
        return cheerBadge10000;
    }

    static LazyLoadedImage *
    getCheerBadge5000()
    {
        return cheerBadge5000;
    }

    static LazyLoadedImage *
    getCheerBadge1000()
    {
        return cheerBadge1000;
    }

    static LazyLoadedImage *
    getCheerBadge100()
    {
        return cheerBadge100;
    }

    static LazyLoadedImage *
    getCheerBadge1()
    {
        return cheerBadge1;
    }

    static LazyLoadedImage *
    getButtonBan()
    {
        return buttonBan;
    }

    static LazyLoadedImage *
    getButtonTimeout()
    {
        return buttonTimeout;
    }

private:
    Resources();

    static LazyLoadedImage *badgeStaff;
    static LazyLoadedImage *badgeAdmin;
    static LazyLoadedImage *badgeGlobalmod;
    static LazyLoadedImage *badgeModerator;
    static LazyLoadedImage *badgeTurbo;
    static LazyLoadedImage *badgeBroadcaster;
    static LazyLoadedImage *badgePremium;

    static LazyLoadedImage *cheerBadge100000;
    static LazyLoadedImage *cheerBadge10000;
    static LazyLoadedImage *cheerBadge5000;
    static LazyLoadedImage *cheerBadge1000;
    static LazyLoadedImage *cheerBadge100;
    static LazyLoadedImage *cheerBadge1;

    static LazyLoadedImage *buttonBan;
    static LazyLoadedImage *buttonTimeout;
};

#endif  // RESOURCES_H
