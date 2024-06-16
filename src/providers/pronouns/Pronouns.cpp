#include "providers/pronouns/Pronouns.hpp"

#include "Application.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "providers/pronouns/alejo/PronounsAlejoApi.hpp"
#include "providers/pronouns/IPronounsApi.hpp"
#include "providers/pronouns/pronoundb/PronounsPronounDbApi.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "singletons/Settings.hpp"

#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace chatterino {

Pronouns::Pronouns()
    : providerPriority{PronounProvider::PRONOUNDB, PronounProvider::ALEJO}
{
    // Initialize providers.
    for (auto const providerId : this->providerPriority)
    {
        this->getProvider(providerId);
    }

    // Start fetching timer.
    QObject::connect(&this->fetchTimer, &QTimer::timeout, [this] {
        this->fetchEnqueued();
    });
    this->fetchTimer.start(30 * 1000);
}

std::shared_ptr<IPronounsApi> Pronouns::getProvider(PronounProvider provider)
{
    // Get provider from provider list if it exists.
    auto iter = this->providers.find(provider);
    if (iter != this->providers.end())
    {
        return iter->second;
    }

    // Else, create and add the provider.
    switch (provider)
    {
        case PronounProvider::ALEJO: {
            auto ptr = std::make_shared<PronounsAlejoApi>();
            this->providers.insert_or_assign(PronounProvider::ALEJO, ptr);
            return ptr;
        }
        case PronounProvider::PRONOUNDB: {
            auto ptr = std::make_shared<PronounsPronounDbApi>();
            this->providers.insert_or_assign(PronounProvider::PRONOUNDB, ptr);
            return ptr;
        }
    }

    return {};
}

void Pronouns::fetchForUsers(
    std::vector<PronounUser> &&allUsers,
    std::unordered_set<PronounProvider> &&doneProviders,
    std::unordered_set<std::string> &&unspecifiedUsers)
{
    // Go through the list of pronoun providers,
    // skipping those that we used already.
    for (auto const providerId : this->providerPriority)
    {
        // Provider already done.
        if (doneProviders.find(providerId) != doneProviders.end())
        {
            continue;
        }

        auto provider = this->getProvider(providerId);
        if (!provider)
        {
            continue;
        }

        // Only fetch pronouns for users that we
        // have not yet fetched the pronouns for.
        auto maxBatchSize = provider->maxBatchSize();
        std::vector<PronounUser> users;
        std::vector<std::string> rejectedUsernames;
        {
            std::size_t i{0};
            std::lock_guard<std::mutex> lock(this->mutex);

            for (auto const &user : allUsers)
            {
                auto iter = this->saved.find(user.username);
                if (iter == this->saved.end())
                {
                    // Only add missing users.
                    if (i < maxBatchSize)
                    {
                        users.push_back(user);
                    }
                    else
                    {
                        rejectedUsernames.push_back(user.username);
                        unspecifiedUsers.erase(user.username);
                    }
                }
                ++i;
            }
        }  // unlock mutex

        // Enqueue rejected usernames for next time.
        enqueueFetch(std::move(rejectedUsernames));

        // Some logging
        if (chatterinoPronouns().isDebugEnabled())
        {
            std::string providerStr;
            switch (providerId)
            {
                case PronounProvider::ALEJO: {
                    providerStr = "alejo.io";
                }
                break;
                case PronounProvider::PRONOUNDB: {
                    providerStr = "pronoundb.org";
                }
                break;
            }

            for (auto const &user : users)
            {
                qCDebug(chatterinoPronouns)
                    << "Fetching pronouns from " << providerStr << " for "
                    << user.username << " (" << user.id << ")";
            }
        }

        // Fetch with the provider, we move some variables, because
        // we return immediately after calling fetch. fetchForUsers
        // is called for the next provider concurrently as soon as
        // the current provider request is done.
        provider->fetch(
            users,
            std::move([this, users, providerId = std::move(providerId),
                       doneProviders = std::move(doneProviders),
                       unspecifiedUsers = std::move(unspecifiedUsers)](
                          IPronounsApi::RequestT::ResultsT &&results) mutable {
                {
                    std::lock_guard<std::mutex> lock(this->mutex);

                    for (auto const &result : results)
                    {
                        // result = (username, maybe pronouns)
                        // If pronouns is nullopt, the api request failed.
                        // If the pronouns are unspecified (UserPronouns()),
                        // the user has no pronouns set with this API.
                        if (result.second)
                        {
                            if (!result.second->isUnspecified())
                            {
                                qCDebug(chatterinoPronouns)
                                    << "Adding pronouns "
                                    << result.second->format() << " for user "
                                    << result.first;
                                this->saved[result.first] = *result.second;
                                unspecifiedUsers.erase(result.first);
                            }
                            else
                            {
                                // We ignore unspecified pronouns and try them
                                // later with the remaining providers. Finally,
                                // when no more providers remain, the unspecified
                                // pronouns are set for the users, so that we
                                // don't fetch them again for every message.
                                unspecifiedUsers.insert(result.first);
                            }
                        }
                    }
                }

                // Try the rest of the providers (recursively).
                doneProviders.insert(providerId);
                this->fetchForUsers(std::move(users), std::move(doneProviders),
                                    std::move(unspecifiedUsers));
            }));

        // Return immediately. We call this function for
        // the rest of the providers in the callback when
        // the provider is done.
        return;
    }

    // No provider left, set unspecified pronouns for
    // those users that the api successfully returned
    // for but did not return any specific pronouns for.
    {
        std::lock_guard<std::mutex> lock(this->mutex);

        for (auto const &username : unspecifiedUsers)
        {
            auto iter = this->saved.find(username);
            if (iter == this->saved.end())
            {
                // Finally, set unspecified pronouns.
                this->saved[username] = UserPronouns();
            }
        }
    }  // unlock mutex
}

void Pronouns::fetchForUsernames(std::vector<std::string> &&allUsernames)
{
    // Don't get pronouns for users who we already have pronouns for.
    std::vector<std::string> usernames;
    usernames.reserve(allUsernames.size());

    {
        std::lock_guard<std::mutex> lock(this->mutex);

        for (auto const &username : allUsernames)
        {
            auto iter = this->saved.find(username);
            if (iter == this->saved.end())
            {
                // No pronouns found yet.
                usernames.push_back(username);
            }
        }
    }  // unlock mutex

    // Get user ids.
    QStringList qUsernames;
    for (auto const &username : usernames)
    {
        qCDebug(chatterinoPronouns) << "Fetching pronouns for " << username;
        qUsernames.append(QString::fromStdString(username));
    }

    if (qUsernames.empty())
    {
        return;
    }

    getHelix()->fetchUsers(
        QStringList(), qUsernames,
        [this](std::vector<HelixUser> helixUsers) {
            // On success.
            std::vector<PronounUser> users;
            for (auto const &helixUser : helixUsers)
            {
                PronounUser user;
                user.id = helixUser.id.toStdString();
                user.username = helixUser.login.toStdString();
                users.push_back(user);
            }
            // Fetch pronouns from providers.
            this->fetchForUsers(std::move(users));
        },
        []() {
            // On error.
        });
}

std::optional<UserPronouns> Pronouns::getForUsername(std::string username)
{
    std::lock_guard<std::mutex> lock(this->mutex);

    auto iter = this->saved.find(username);
    if (iter == this->saved.end())
    {
        return {};
    }

    return iter->second;
}

void Pronouns::enqueueFetch(std::vector<std::string> usernames)
{
    if (!getSettings()->showPronouns)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(this->mutex);

    for (auto const &username : usernames)
    {
        auto iter = this->saved.find(username);
        if (iter == this->saved.end())
        {
            qCDebug(chatterinoPronouns) << "Enqueueing " << username;
            this->queuedFetches.push_back(username);
        }
    }
}

void Pronouns::enqueueFetchFront(std::vector<std::string> usernames)
{
    if (!getSettings()->showPronouns)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(this->mutex);

    for (auto const &username : usernames)
    {
        auto iter = this->saved.find(username);
        if (iter == this->saved.end())
        {
            qCDebug(chatterinoPronouns)
                << "Enqueueing (at the front)" << username;
            this->queuedFetches.push_front(username);
        }
    }
}

void Pronouns::fetchEnqueued()
{
    if (!getSettings()->showPronouns)
    {
        return;
    }

    std::size_t maxDeque = 0;
    for (auto const providerId : this->providerPriority)
    {
        auto provider = this->getProvider(providerId);
        if (!provider)
        {
            continue;
        }

        maxDeque = provider->maxBatchSize();
        break;
    }

    std::vector<std::string> usernames;
    {
        std::lock_guard<std::mutex> lock(this->mutex);

        for (std::size_t i{0}; !this->queuedFetches.empty() && i < maxDeque;
             ++i)
        {
            std::string username{this->queuedFetches.front()};
            this->queuedFetches.pop_front();
            usernames.push_back(username);
        }
    }

    if (usernames.empty())
    {
        return;
    }

    qCDebug(chatterinoPronouns) << "Fetching enqueued";
    getIApp()->getPronouns()->fetchForUsernames(std::move(usernames));
}

}  // namespace chatterino
