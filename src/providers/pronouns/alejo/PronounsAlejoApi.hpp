#pragma once

#include "providers/pronouns/UserPronouns.hpp"

#include <QJsonObject>
#include <QString>

#include <functional>
#include <optional>
#include <shared_mutex>
#include <unordered_map>

namespace chatterino::pronouns {

class AlejoApi
{
public:
    AlejoApi();

    /// Fetch the user's pronouns from the alejo.io API
    ///
    /// onDone can be invoked from any thread
    ///
    /// The argument is std::nullopt if and only if the request failed.
    void fetch(const QString &username,
               const std::function<void(std::optional<UserPronouns>)> &onDone);

private:
    void loadAvailablePronouns();

    std::shared_mutex mutex;
    /// Maps alejo.io pronoun IDs to human readable representation like `they/them` or `other`
    std::unordered_map<QString, QString> pronouns;

    /// Parse a pronoun definition from the /users endpoint into a finished UserPronouns
    UserPronouns parsePronoun(const QJsonObject &object);
};

}  // namespace chatterino::pronouns
