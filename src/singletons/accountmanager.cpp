#include "singletons/accountmanager.hpp"

namespace chatterino {
namespace singletons {

namespace {

inline QString getEnvString(const char *target)
{
    char *val = std::getenv(target);
    if (val == nullptr) {
        return QString();
    }

    return QString(val);
}

}  // namespace

AccountManager &AccountManager::getInstance()
{
    static AccountManager instance;
    return instance;
}

void AccountManager::load()
{
    this->Twitch.load();
}

}  // namespace singletons
}  // namespace chatterino
