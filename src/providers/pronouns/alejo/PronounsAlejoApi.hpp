#pragma once

#include "providers/pronouns/common/PronounsApiRequest.hpp"
#include "providers/pronouns/IPronounsApi.hpp"
#include "providers/pronouns/PronounUser.hpp"
#include "providers/pronouns/UserPronouns.hpp"

#include <mutex>

namespace chatterino {

class PronounsAlejoApi : public IPronounsApi
{
private:
    static std::mutex mutex;
    static std::optional<std::unordered_map<std::string, std::string>>
        pronounsFromId;
    static UserPronouns parse(QJsonObject);
    inline static const std::string API_URL =
        "https://api.pronouns.alejo.io/v1";
    inline static const std::string API_USERS = "/users";
    inline static const std::string API_PRONOUNS = "/pronouns";

public:
    explicit PronounsAlejoApi();
    virtual void fetch(std::vector<PronounUser> users,
                       RequestT::CallbackT &&onDone) override;
    virtual std::size_t maxBatchSize() override;
};

}  // namespace chatterino
