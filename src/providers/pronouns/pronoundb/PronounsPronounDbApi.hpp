#pragma once

#include "providers/pronouns/common/PronounsApiRequest.hpp"
#include "providers/pronouns/IPronounsApi.hpp"
#include "providers/pronouns/PronounUser.hpp"
#include "providers/pronouns/UserPronouns.hpp"

#include <QJsonArray>

namespace chatterino {

class PronounsPronounDbApi : public IPronounsApi
{
public:
    using UserId = std::string;

private:
    // Converts pronoundb "sets" to UserPronouns.
    static inline UserPronouns setsToPronouns(std::vector<std::string> sets);
    static std::optional<UserPronouns> parseUser(QJsonObject &&);
    static std::vector<std::pair<UserId, UserPronouns>> parse(QJsonObject &&);
    static std::string makeRequest(std::vector<PronounUser> &users);

public:
    virtual void fetch(std::vector<PronounUser> users,
                       RequestT::CallbackT &&onDone) override;
    virtual std::size_t maxBatchSize() override;
};

}  // namespace chatterino
