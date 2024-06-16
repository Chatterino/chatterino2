#pragma once

#include "providers/pronouns/common/PronounsApiRequest.hpp"
#include "providers/pronouns/PronounUser.hpp"
#include "providers/pronouns/UserPronouns.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace chatterino {

class IPronounsApi
{
public:
    // (username, pronouns)
    // pronouns may be std::nullopt, signaling an error during the fetch.
    // pronouns may be UserPronouns(), signaling successful fetching and parsing,
    // but that the user has not set any pronouns.
    // If pronouns is std::nullopt.
    using ResultT = std::pair<std::string, std::optional<UserPronouns>>;
    using RequestT = PronounsApiRequest<ResultT>;

    // Fetch pronouns from the API.
    virtual void fetch(std::vector<PronounUser> users,
                       RequestT::CallbackT &&onDone){};
    // Gets the maximum batch size for a single request.
    virtual std::size_t maxBatchSize()
    {
        return 0;
    };
};

}  // namespace chatterino
