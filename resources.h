#ifndef RESOURCES_H
#define RESOURCES_H

#include <lazyloadedimage.h>

class Resources
{
public:
    static void load();

    // badges
    static LazyLoadedImage *
    badgeStaff()
    {
        return m_badgeStaff;
    }

    static LazyLoadedImage *
    badgeAdmin()
    {
        return m_badgeAdmin;
    }

    static LazyLoadedImage *
    badgeGlobalmod()
    {
        return m_badgeGlobalmod;
    }

    static LazyLoadedImage *
    badgeModerator()
    {
        return m_badgeModerator;
    }

    static LazyLoadedImage *
    badgeTurbo()
    {
        return m_badgeTurbo;
    }

    static LazyLoadedImage *
    badgeBroadcaster()
    {
        return m_badgeBroadcaster;
    }

    static LazyLoadedImage *
    badgePremium()
    {
        return m_badgePremium;
    }

    // cheer badges
    static LazyLoadedImage *
    cheerBadge100000()
    {
        return m_cheerBadge100000;
    }

    static LazyLoadedImage *
    cheerBadge10000()
    {
        return m_cheerBadge10000;
    }

    static LazyLoadedImage *
    cheerBadge5000()
    {
        return m_cheerBadge5000;
    }

    static LazyLoadedImage *
    cheerBadge1000()
    {
        return m_cheerBadge1000;
    }

    static LazyLoadedImage *
    cheerBadge100()
    {
        return m_cheerBadge100;
    }

    static LazyLoadedImage *
    cheerBadge1()
    {
        return m_cheerBadge1;
    }

    static LazyLoadedImage *
    buttonBan()
    {
        return m_buttonBan;
    }

    static LazyLoadedImage *
    buttonTimeout()
    {
        return m_buttonTimeout;
    }

private:
    Resources();

    static LazyLoadedImage *m_badgeStaff;
    static LazyLoadedImage *m_badgeAdmin;
    static LazyLoadedImage *m_badgeGlobalmod;
    static LazyLoadedImage *m_badgeModerator;
    static LazyLoadedImage *m_badgeTurbo;
    static LazyLoadedImage *m_badgeBroadcaster;
    static LazyLoadedImage *m_badgePremium;

    static LazyLoadedImage *m_cheerBadge100000;
    static LazyLoadedImage *m_cheerBadge10000;
    static LazyLoadedImage *m_cheerBadge5000;
    static LazyLoadedImage *m_cheerBadge1000;
    static LazyLoadedImage *m_cheerBadge100;
    static LazyLoadedImage *m_cheerBadge1;

    static LazyLoadedImage *m_buttonBan;
    static LazyLoadedImage *m_buttonTimeout;
};

#endif  // RESOURCES_H
