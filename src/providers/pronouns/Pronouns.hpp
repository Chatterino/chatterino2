#pragma once

#include "providers/pronouns/alejo/PronounsAlejoApi.hpp"
#include "providers/pronouns/UserPronouns.hpp"

#include <shared_mutex>
#include <optional>
#include <unordered_map>

namespace chatterino::pronouns {

class Pronouns
{
private:
    // mutex for editing the saved map.
    std::shared_mutex mutex;
    // Login name -> Pronouns
    std::unordered_map<QString, UserPronouns> saved;
    AlejoApi alejoApi;

public:
    Pronouns() = default;

    void fetch(const QString &user,
               const std::function<void(UserPronouns)> &callbackSuccess,
               const std::function<void()> &callbackFail);

    // Retrieve cached pronouns for user.
    std::optional<UserPronouns> getForUsername(const QString &username);
};

}  // namespace chatterino::pronouns
