#pragma once

#include "providers/pronouns/alejo/PronounsAlejoApi.hpp"
#include "providers/pronouns/UserPronouns.hpp"

#include <QTimer>

#include <mutex>
#include <optional>
#include <unordered_map>

namespace chatterino::Pronouns {

enum class PronounProvider { PRONOUNDB, ALEJO };

class Pronouns
{
private:
    // mutex for editing the saved map.
    std::mutex mutex;
    // Login name -> Pronouns
    std::unordered_map<QString, UserPronouns> saved;
    AlejoApi alejoApi;

public:
    Pronouns() = default;

    void fetch(const QString &user,
               std::function<void(UserPronouns)> callbackSuccess,
               std::function<void()> callbackFail);

    // Retrieve cached pronouns for user.
    std::optional<UserPronouns> getForUsername(const QString &username);
};

}  // namespace chatterino::Pronouns
