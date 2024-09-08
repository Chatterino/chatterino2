#pragma once

#include "providers/pronouns/alejo/PronounsAlejoApi.hpp"
#include "providers/pronouns/UserPronouns.hpp"

#include <functional>
#include <optional>
#include <shared_mutex>
#include <unordered_map>

namespace chatterino::pronouns {

class Pronouns
{
public:
    void getUserPronoun(
        const QString &username,
        const std::function<void(UserPronouns)> &callbackSuccess,
        const std::function<void()> &callbackFail);

private:
    // Retrieve cached pronouns for user.
    std::optional<UserPronouns> getCachedUserPronoun(const QString &username);

    // mutex for editing the saved map.
    std::shared_mutex mutex;
    // Login name -> Pronouns
    std::unordered_map<QString, UserPronouns> saved;
    AlejoApi alejoApi;
};

}  // namespace chatterino::pronouns
