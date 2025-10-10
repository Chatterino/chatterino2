#include "controllers/emotes/BuiltinEmoteProvider.hpp"

#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "controllers/emotes/EmoteChannel.hpp"
#include "controllers/emotes/EmoteHolder.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "util/Functional.hpp"
#include "util/Helpers.hpp"

#include <QJsonDocument>
#include <QJsonValue>

namespace chatterino {

using namespace Qt::Literals;

BuiltinEmoteProvider::BuiltinEmoteProvider(BoolSetting *globalSetting,
                                           QString globalUrl,
                                           BoolSetting *channelSetting,
                                           QString name, QString id,
                                           uint32_t priority)
    : EmoteProvider(std::move(name), std::move(id), priority)
    , globalSetting(globalSetting)
    , globalUrl(std::move(globalUrl))
    , channelSetting(channelSetting)
{
}

void BuiltinEmoteProvider::initialize()
{
    globalSetting->connect(
        [this] {
            this->reloadGlobalEmotes([name = this->name()](const auto &res) {
                if (!res)
                {
                    qCWarning(chatterinoEmotes)
                        << "Failed to load global" << name << "emotes"
                        << res.error();
                }
            });
        },
        this->signals);
    channelSetting->connect(
        [this](bool value) {
            this->channelEmotesEnabled.invoke(value);
        },
        this->signals, false);
}

void BuiltinEmoteProvider::loadChannelEmotes(
    const std::shared_ptr<EmoteChannel> &channel,
    std::function<void(ExpectedStr<EmoteLoadResult>)> onDone,
    LoadChannelArgs args)
{
    std::shared_ptr<TwitchChannel> twitch =
        std::dynamic_pointer_cast<TwitchChannel>(channel);
    if (!twitch)
    {
        onDone(EmoteLoadResult{.emotes = nullptr});
        return;
    }

    if (!args.manualRefresh)
    {
        readProviderEmotesCache(
            twitch->roomId(), this->id(), [&](const auto &jv) {
                auto emotes = this->parseChannelEmotes(*twitch, jv);
                if (emotes)
                {
                    onDone(EmoteLoadResult{
                        .emotes = std::make_shared<const EmoteMap>(
                            *std::move(emotes)),
                        .isPreemptiveCached = true,
                    });
                }
            });
    }

    NetworkRequest(this->channelEmotesUrl(*twitch))
        .timeout(20000)
        .onSuccess(weakGuarded(
            [onDone](const std::shared_ptr<TwitchChannel> &twitch,
                     const std::shared_ptr<BuiltinEmoteProvider> &self,
                     const auto &result) {
                auto emotes =
                    self->parseChannelEmotes(*twitch, result.parseJson());
                if (emotes)
                {
                    writeProviderEmotesCache(twitch->roomId(), self->id(),
                                             result.getData());
                    onDone(EmoteLoadResult{
                        .emotes = std::make_shared<const EmoteMap>(
                            *std::move(emotes)),
                    });
                }
                else
                {
                    onDone(makeUnexpected(u"Failed to parse response"_s));
                }
            },
            twitch, this->weak_from_this()))
        .onError([onDone](const auto &result) {
            if (result.status() == 404)
            {
                onDone(EmoteLoadResult{.emotes = nullptr});
                return;
            }

            onDone(makeUnexpected(result.formatError()));
        })
        .execute();
}

void BuiltinEmoteProvider::reloadGlobalEmotes(
    std::function<void(ExpectedStr<void>)> onDone)
{
    if (!this->globalSetting->getValue())
    {
        this->globalEmotes_ = EMPTY_EMOTE_MAP;
        onDone({});
        return;
    }

    NetworkRequest(this->globalUrl)
        .timeout(30000)
        .onSuccess(weakGuarded(
            [onDone](const std::shared_ptr<BuiltinEmoteProvider> &self,
                     const auto &result) {
                if (!self->globalSetting->getValue())
                {
                    return;  // the user might've changed their mind
                }

                writeProviderEmotesCache("global", self->id(),
                                         result.getData());
                auto emotes = self->parseGlobalEmotes(result.parseJsonValue());
                if (emotes)
                {
                    self->globalEmotes_ =
                        std::make_shared<EmoteMap>(*std::move(emotes));
                }
            },
            this->weak_from_this()))
        .onError(weakGuarded(
            [onDone](const std::shared_ptr<BuiltinEmoteProvider> &self,
                     const auto &result) {
                if (!self->globalSetting->getValue())
                {
                    return;
                }

                qCWarning(chatterinoEmoji)
                    << "Failed to fetch global BTTV emotes. "
                    << result.formatError();

                readProviderEmotesCache(
                    "global", "betterttv", [&](const auto &jv) {
                        auto emotes = self->parseGlobalEmotes(jv);
                        if (emotes)
                        {
                            self->globalEmotes_ =
                                std::make_shared<EmoteMap>(*std::move(emotes));
                        }
                    });
            },
            this->weak_from_this()))
        .execute();
}

bool BuiltinEmoteProvider::supportsChannel(EmoteChannel *channel)
{
    return dynamic_cast<TwitchChannel *>(channel) != nullptr;
}

bool BuiltinEmoteProvider::hasChannelEmotes() const
{
    return this->channelSetting->getValue();
}

bool BuiltinEmoteProvider::hasGlobalEmotes() const
{
    return this->globalSetting->getValue();
}

bool BuiltinEmoteProvider::addChannelEmote(EmoteHolder &holder, EmotePtr emote)
{
    auto *item = holder.itemByProvider(this->weak_from_this());
    if (!item)
    {
        return false;
    }

    EmoteMap copy = *item->emotes;
    EmoteName name = emote->name;
    copy[name] = std::move(emote);
    item->emotes = std::make_shared<const EmoteMap>(std::move(copy));
    return true;
}

std::optional<std::pair<EmotePtr, EmotePtr>>
    BuiltinEmoteProvider::updateChannelEmote(
        EmoteHolder &holder, const QString &id, const QString &nameHint,
        FunctionRef<EmotePtr(const EmotePtr &)> createUpdatedEmote)
{
    auto *item = holder.itemByProvider(this->weak_from_this());
    if (!item)
    {
        return std::nullopt;
    }

    EmoteMap copy = *item->emotes;

    // Step 1: remove the existing emote
    auto it = copy.findEmote(nameHint, id);
    if (it == copy.end())
    {
        // We already copied the map at this point and are now discarding the copy.
        // This is fine, because this case should be really rare.
        return std::nullopt;
    }
    auto oldEmotePtr = it->second;
    copy.erase(it);

    // Step 2: update the emote
    auto newEmotePtr = createUpdatedEmote(oldEmotePtr);
    if (!newEmotePtr)
    {
        // The emote wasn't actually updated
        return std::nullopt;
    }

    copy[newEmotePtr->name] = newEmotePtr;
    item->emotes = std::make_shared<const EmoteMap>(std::move(copy));

    return std::make_pair(oldEmotePtr, newEmotePtr);
}

EmotePtr BuiltinEmoteProvider::removeChannelEmote(EmoteHolder &holder,
                                                  const QString &id,
                                                  const QString &nameHint)
{
    auto *item = holder.itemByProvider(this->weak_from_this());
    if (!item)
    {
        return nullptr;
    }

    EmoteMap copy = *item->emotes;
    auto it = copy.findEmote(nameHint, id);
    if (it == copy.end())
    {
        // We already copied the map at this point and are now discarding the copy.
        // This is fine, because this case should be really rare.
        return nullptr;
    }
    auto emote = it->second;
    copy.erase(it);
    item->emotes = std::make_shared<const EmoteMap>(std::move(copy));
    return emote;
}

}  // namespace chatterino
