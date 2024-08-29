#pragma once

#include "providers/pronouns/UserPronouns.hpp"

#include <QJsonObject>
#include <QString>

#include <mutex>
#include <optional>
#include <shared_mutex>

namespace chatterino::pronouns {

class AlejoApi
{
public:
    explicit AlejoApi();
    /** Fetches pronouns from the alejo.io API for a username and calls onDone when done.
        onDone can be invoked from any thread. The argument is std::nullopt if and only if the request failed.
     */
    void fetch(const QString &username,
               std::function<void(std::optional<UserPronouns>)> onDone);

private:
    std::shared_mutex mutex;
    /** A map from alejo.io ids to human readable representation like theythem -> they/them, other -> other. */
    std::optional<std::unordered_map<QString, QString>> pronounsFromId =
        std::nullopt;
    UserPronouns parse(const QJsonObject &);
    inline static const QString API_URL = "https://api.pronouns.alejo.io/v1";
    inline static const QString API_USERS = "/users";
    inline static const QString API_PRONOUNS = "/pronouns";
};

}  // namespace chatterino::pronouns
