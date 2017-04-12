#include "usermanager.h"

namespace chatterino {

AccountManager AccountManager::instance;

AccountManager::AccountManager()
    : _anon("justinfan64537", "", "")
{
}

twitch::TwitchUser &AccountManager::getAnon()
{
    return _anon;
}
}
