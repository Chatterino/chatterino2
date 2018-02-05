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

AccountManager::AccountManager()
{
}

AccountManager &AccountManager::getInstance()
{
    static AccountManager instance;
    return instance;
}

void AccountManager::load()
{
    this->Twitch.reloadUsers();

    auto currentUser = this->Twitch.findUserByUsername(
        QString::fromStdString(this->Twitch.currentUsername.getValue()));

    if (currentUser) {
        this->Twitch.currentUser = currentUser;
    } else {
        this->Twitch.currentUser = this->Twitch.anonymousUser;
    }

    this->Twitch.userChanged.invoke();
}

}  // namespace singletons
}  // namespace chatterino
