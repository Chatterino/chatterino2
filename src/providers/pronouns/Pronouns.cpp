#include "providers/pronouns/Pronouns.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "providers/pronouns/alejo/PronounsAlejoApi.hpp"
#include "providers/pronouns/UserPronouns.hpp"

#include <mutex>
#include <unordered_map>

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
const auto &LOG = chatterinoPronouns;

}  // namespace

namespace chatterino::pronouns {

void Pronouns::getUserPronoun(
    const QString &username,
    const std::function<void(UserPronouns)> &callbackSuccess,
    const std::function<void()> &callbackFail)
{
    // Only fetch pronouns if we haven't fetched before.
    auto cachedPronoun = this->getCachedUserPronoun(username);
    if (cachedPronoun.has_value())
    {
        callbackSuccess(*cachedPronoun);
        return;
    }

    this->alejoApi.fetch(username, [this, callbackSuccess, callbackFail,
                                    username](const auto &oUserPronoun) {
        if (!oUserPronoun.has_value())
        {
            callbackFail();
            return;
        }

        const auto &userPronoun = *oUserPronoun;

        qCDebug(LOG) << "Caching pronoun" << userPronoun.format() << "for user"
                     << username;
        {
            std::unique_lock lock(this->mutex);
            this->saved[username] = userPronoun;
        }

        callbackSuccess(userPronoun);
    });
}

std::optional<UserPronouns> Pronouns::getCachedUserPronoun(
    const QString &username)
{
    std::shared_lock lock(this->mutex);
    auto it = this->saved.find(username);
    if (it != this->saved.end())
    {
        return {it->second};
    }
    return {};
}

}  // namespace chatterino::pronouns
