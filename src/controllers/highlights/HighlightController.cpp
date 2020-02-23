#include "HighlightController.hpp"

#include "Application.hpp"
#include "util/PersistSignalVector.hpp"

namespace chatterino {

void HighlightController::initialize(Settings &settings, Paths &paths)
{
    assert(!this->initialized_);
    this->initialized_ = true;

    persist(this->phrases, "/highlighting/highlights");
    persist(this->blacklistedUsers, "/highlighting/blacklist");
    persist(this->highlightedUsers, "/highlighting/users");
}

bool HighlightController::isHighlightedUser(const QString &username)
{
    for (const auto &highlightedUser : this->highlightedUsers)
    {
        if (highlightedUser.isMatch(username))
            return true;
    }

    return false;
}

bool HighlightController::blacklistContains(const QString &username)
{
    for (const auto &blacklistedUser : *this->blacklistedUsers.readOnly())
    {
        if (blacklistedUser.isMatch(username))
            return true;
    }

    return false;
}

}  // namespace chatterino
