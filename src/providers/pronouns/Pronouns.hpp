#pragma once

#include "providers/pronouns/IPronounsApi.hpp"
#include "providers/pronouns/PronounUser.hpp"
#include "providers/pronouns/UserPronouns.hpp"

#include <QTimer>

#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace chatterino {

enum class PronounProvider { PRONOUNDB, ALEJO };

class Pronouns
{
private:
    // mutex for editing the saved map.
    std::mutex mutex;
    // Login name -> Pronouns
    std::unordered_map<std::string, UserPronouns> saved;
    std::unordered_map<PronounProvider, std::shared_ptr<IPronounsApi>>
        providers;
    std::vector<PronounProvider> providerPriority;
    std::deque<std::string> queuedFetches;

    // Get pronoun provider from enum.
    // Creates, adds to list, and returns the provider if it was not yet created.
    std::shared_ptr<IPronounsApi> getProvider(PronounProvider);

    // Goes through the list of providers in order of priority and fetches the users' pronouns.
    // Each pronoun provider may fetch concurrently.
    void fetchForUsers(std::vector<PronounUser> &&users,
                       std::unordered_set<PronounProvider> &&doneProviders = {},
                       std::unordered_set<std::string> &&unspecifiedUsers = {});

    // Fetches and saves pronouns for users to the cache.
    // Pronouns are saved concurrently and may be retrieved from the cache using getForUsername.
    void fetchForUsernames(std::vector<std::string> &&usernames);

    QTimer fetchTimer;
    void fetchEnqueued();

public:
    Pronouns();

    void enqueueFetch(std::vector<std::string> usernames);
    void enqueueFetchFront(std::vector<std::string> usernames);

    // Retrieve cached pronouns for user.
    std::optional<UserPronouns> getForUsername(std::string username);
};

}  // namespace chatterino
