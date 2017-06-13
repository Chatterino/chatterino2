#pragma once

#include "messages/lazyloadedimage.hpp"

namespace chatterino {

class EmoteManager;
class WindowManager;

class Resources
{
public:
    explicit Resources(EmoteManager &emoteManager, WindowManager &windowManager);

    messages::LazyLoadedImage *badgeStaff;
    messages::LazyLoadedImage *badgeAdmin;
    messages::LazyLoadedImage *badgeGlobalModerator;
    messages::LazyLoadedImage *badgeModerator;
    messages::LazyLoadedImage *badgeTurbo;
    messages::LazyLoadedImage *badgeBroadcaster;
    messages::LazyLoadedImage *badgePremium;
    messages::LazyLoadedImage *badgeVerified;

    messages::LazyLoadedImage *cheerBadge100000;
    messages::LazyLoadedImage *cheerBadge10000;
    messages::LazyLoadedImage *cheerBadge5000;
    messages::LazyLoadedImage *cheerBadge1000;
    messages::LazyLoadedImage *cheerBadge100;
    messages::LazyLoadedImage *cheerBadge1;

    messages::LazyLoadedImage *buttonBan;
    messages::LazyLoadedImage *buttonTimeout;
};

}  // namespace chatterino
