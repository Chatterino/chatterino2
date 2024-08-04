#include "providers/twitch/TwitchAccount.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "common/Env.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "messages/Emote.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/irc/IrcMessageBuilder.hpp"
#include "providers/IvrApi.hpp"
#include "providers/seventv/SeventvAPI.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "singletons/Emotes.hpp"
#include "util/CancellationToken.hpp"
#include "util/Helpers.hpp"
#include "util/QStringHash.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <boost/functional/hash.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <QThread>

#include <ranges>

namespace {

using namespace chatterino;

size_t hashEmoteSet(const TwitchEmoteSet &set)
{
    auto &&rng = set.emotes | std::views::transform([](const auto &it) {
                     return it->id;
                 });
    return boost::hash_unordered_range(std::begin(rng), std::end(rng));
}

}  // namespace

namespace chatterino {

TwitchAccount::TwitchAccount(const QString &username, const QString &oauthToken,
                             const QString &oauthClient, const QString &userID)
    : Account(ProviderId::Twitch)
    , oauthClient_(oauthClient)
    , oauthToken_(oauthToken)
    , userName_(username)
    , userId_(userID)
    , isAnon_(username == ANONYMOUS_USERNAME)
    , emoteSetCache_(new decltype(this->emoteSetCache_)::element_type())
{
}

TwitchAccount::~TwitchAccount() = default;

QString TwitchAccount::toString() const
{
    return this->getUserName();
}

const QString &TwitchAccount::getUserName() const
{
    return this->userName_;
}

const QString &TwitchAccount::getOAuthClient() const
{
    return this->oauthClient_;
}

const QString &TwitchAccount::getOAuthToken() const
{
    return this->oauthToken_;
}

const QString &TwitchAccount::getUserId() const
{
    return this->userId_;
}

QColor TwitchAccount::color()
{
    return this->color_.get();
}

void TwitchAccount::setColor(QColor color)
{
    this->color_.set(std::move(color));
}

bool TwitchAccount::setOAuthClient(const QString &newClientID)
{
    if (this->oauthClient_.compare(newClientID) == 0)
    {
        return false;
    }

    this->oauthClient_ = newClientID;

    return true;
}

bool TwitchAccount::setOAuthToken(const QString &newOAuthToken)
{
    if (this->oauthToken_.compare(newOAuthToken) == 0)
    {
        return false;
    }

    this->oauthToken_ = newOAuthToken;

    return true;
}

bool TwitchAccount::isAnon() const
{
    return this->isAnon_;
}

void TwitchAccount::loadBlocks()
{
    assertInGuiThread();

    auto token = CancellationToken(false);
    this->blockToken_ = token;
    this->ignores_.clear();
    this->ignoresUserIds_.clear();

    getHelix()->loadBlocks(
        getApp()->getAccounts()->twitch.getCurrent()->userId_,
        [this](const std::vector<HelixBlock> &blocks) {
            assertInGuiThread();

            for (const HelixBlock &block : blocks)
            {
                TwitchUser blockedUser;
                blockedUser.fromHelixBlock(block);
                this->ignores_.insert(blockedUser);
                this->ignoresUserIds_.insert(blockedUser.id);
            }
        },
        [](auto error) {
            qCWarning(chatterinoTwitch).noquote()
                << "Fetching blocks failed:" << error;
        },
        std::move(token));
}

void TwitchAccount::blockUser(const QString &userId, const QObject *caller,
                              std::function<void()> onSuccess,
                              std::function<void()> onFailure)
{
    getHelix()->blockUser(
        userId, caller,
        [this, userId, onSuccess = std::move(onSuccess)] {
            assertInGuiThread();

            TwitchUser blockedUser;
            blockedUser.id = userId;
            this->ignores_.insert(blockedUser);
            this->ignoresUserIds_.insert(blockedUser.id);
            onSuccess();
        },
        std::move(onFailure));
}

void TwitchAccount::unblockUser(const QString &userId, const QObject *caller,
                                std::function<void()> onSuccess,
                                std::function<void()> onFailure)
{
    getHelix()->unblockUser(
        userId, caller,
        [this, userId, onSuccess = std::move(onSuccess)] {
            assertInGuiThread();

            TwitchUser ignoredUser;
            ignoredUser.id = userId;
            this->ignores_.erase(ignoredUser);
            this->ignoresUserIds_.erase(ignoredUser.id);
            onSuccess();
        },
        std::move(onFailure));
}

const std::unordered_set<TwitchUser> &TwitchAccount::blocks() const
{
    assertInGuiThread();
    return this->ignores_;
}

const std::unordered_set<QString> &TwitchAccount::blockedUserIds() const
{
    assertInGuiThread();
    return this->ignoresUserIds_;
}

// AutoModActions
void TwitchAccount::autoModAllow(const QString msgID, ChannelPtr channel)
{
    getHelix()->manageAutoModMessages(
        this->getUserId(), msgID, "ALLOW",
        [] {
            // success
        },
        [channel](auto error) {
            // failure
            QString errorMessage("Failed to allow AutoMod message - ");

            switch (error)
            {
                case HelixAutoModMessageError::MessageAlreadyProcessed: {
                    errorMessage += "message has already been processed.";
                }
                break;

                case HelixAutoModMessageError::UserNotAuthenticated: {
                    errorMessage += "you need to re-authenticate.";
                }
                break;

                case HelixAutoModMessageError::UserNotAuthorized: {
                    errorMessage +=
                        "you don't have permission to perform that action";
                }
                break;

                case HelixAutoModMessageError::MessageNotFound: {
                    errorMessage += "target message not found.";
                }
                break;

                // This would most likely happen if the service is down, or if the JSON payload returned has changed format
                case HelixAutoModMessageError::Unknown:
                default: {
                    errorMessage += "an unknown error occured.";
                }
                break;
            }

            channel->addSystemMessage(errorMessage);
        });
}

void TwitchAccount::autoModDeny(const QString msgID, ChannelPtr channel)
{
    getHelix()->manageAutoModMessages(
        this->getUserId(), msgID, "DENY",
        [] {
            // success
        },
        [channel](auto error) {
            // failure
            QString errorMessage("Failed to deny AutoMod message - ");

            switch (error)
            {
                case HelixAutoModMessageError::MessageAlreadyProcessed: {
                    errorMessage += "message has already been processed.";
                }
                break;

                case HelixAutoModMessageError::UserNotAuthenticated: {
                    errorMessage += "you need to re-authenticate.";
                }
                break;

                case HelixAutoModMessageError::UserNotAuthorized: {
                    errorMessage +=
                        "you don't have permission to perform that action";
                }
                break;

                case HelixAutoModMessageError::MessageNotFound: {
                    errorMessage += "target message not found.";
                }
                break;

                // This would most likely happen if the service is down, or if the JSON payload returned has changed format
                case HelixAutoModMessageError::Unknown:
                default: {
                    errorMessage += "an unknown error occured.";
                }
                break;
            }

            channel->addSystemMessage(errorMessage);
        });
}

const QString &TwitchAccount::getSeventvUserID() const
{
    return this->seventvUserID_;
}

void TwitchAccount::loadSeventvUserID()
{
    if (this->isAnon())
    {
        return;
    }
    if (!this->seventvUserID_.isEmpty())
    {
        return;
    }

    auto *seventv = getApp()->getSeventvAPI();
    if (!seventv)
    {
        qCWarning(chatterinoSeventv)
            << "Not loading 7TV User ID because the 7TV API is not initialized";
        return;
    }

    seventv->getUserByTwitchID(
        this->getUserId(),
        [this](const auto &json) {
            const auto id = json["user"]["id"].toString();
            if (!id.isEmpty())
            {
                this->seventvUserID_ = id;
            }
        },
        [](const auto &result) {
            qCDebug(chatterinoSeventv)
                << "Failed to load 7TV user-id:" << result.formatError();
        });
}

void TwitchAccount::deduplicateEmoteSets(TwitchEmoteSetMap &sets)
{
    std::lock_guard guard(this->emoteCacheMutex_);

    bool anyModification = false;
    for (auto &it : sets)
    {
        if (it.second->isFollower)
        {
            continue;
        }
        auto hash = hashEmoteSet(*it.second);
        auto cachedIt = this->emoteSetCache_->find(it.first);
        if (cachedIt != this->emoteSetCache_->end() &&
            cachedIt->second.hash == hash)
        {
            // same hash - replace with cached value
            it.second = cachedIt->second.ptr;
            continue;
        }

        // not yet cached or mismatch - add/update to cache
        this->emoteSetCache_->emplace(it.first, CachedEmoteSet{
                                                    .hash = hash,
                                                    .ptr = it.second,
                                                });
        anyModification = true;
        qCDebug(chatterinoTwitch)
            << "Updated cache for emote set" << it.first.string;
    }

    if (anyModification)
    {
        // rebuild non-local emote map
        EmoteMap map;
        for (const auto &[id, emotes] : *this->emoteSetCache_)
        {
            for (auto emote : emotes.ptr->emotes)
            {
                map.emplace(emote->name, std::move(emote));
            }
        }
        this->cachedNonLocalEmotes_.set(
            std::make_shared<EmoteMap>(std::move(map)));
    }
}

std::shared_ptr<const EmoteMap> TwitchAccount::cachedNonLocalEmotes() const
{
    return this->cachedNonLocalEmotes_.get();
}

}  // namespace chatterino
