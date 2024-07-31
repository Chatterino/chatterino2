#pragma once

#include "providers/pronouns/UserPronouns.hpp"

#include <mutex>
#include <optional>

namespace chatterino::Pronouns {

class AlejoApi
{
private:
    std::mutex mutex;
    std::optional<std::unordered_map<QString, QString>> pronounsFromId =
        std::nullopt;
    UserPronouns parse(QJsonObject);
    inline static const QString API_URL = "https://api.pronouns.alejo.io/v1";
    inline static const QString API_USERS = "/users";
    inline static const QString API_PRONOUNS = "/pronouns";

public:
    explicit AlejoApi();
    void fetch(const QString &user,
               std::function<void(std::optional<UserPronouns>)> onDone);
};

}  // namespace chatterino::Pronouns
