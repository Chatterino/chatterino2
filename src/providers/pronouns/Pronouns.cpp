#include "providers/pronouns/Pronouns.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "providers/pronouns/alejo/PronounsAlejoApi.hpp"
#include "providers/pronouns/UserPronouns.hpp"

#include <mutex>
#include <string>
#include <unordered_map>

namespace chatterino::pronouns {

void Pronouns::fetch(const QString &username,
                     const std::function<void(UserPronouns)> &callbackSuccess,
                     const std::function<void()> &callbackFail)
{
    // Only fetch pronouns if we haven't fetched before.
    {
        std::shared_lock<std::shared_mutex> lock(this->mutex);

        auto iter = this->saved.find(username);
        if (iter != this->saved.end())
        {
            callbackSuccess(iter->second);
            return;
        }
    }  // unlock mutex

    qCDebug(chatterinoPronouns)
        << "Fetching pronouns from alejo.io for " << username;

    alejoApi.fetch(username, [this, callbackSuccess, callbackFail,
                              username](std::optional<UserPronouns> result) {
        if (result.has_value())
        {
            {
                std::unique_lock<std::shared_mutex> lock(this->mutex);
                this->saved[username] = *result;
            }  // unlock mutex
            qCDebug(chatterinoPronouns)
                << "Adding pronouns " << result->format() << " for user "
                << username;
            callbackSuccess(*result);
        }
        else
        {
            callbackFail();
        }
    });
}

std::optional<UserPronouns> Pronouns::getForUsername(const QString &username)
{
    std::shared_lock<std::shared_mutex> lock(this->mutex);
    auto it = this->saved.find(username);
    if (it != this->saved.end())
    {
        return {it->second};
    }
    return {};
}

}  // namespace chatterino::pronouns
