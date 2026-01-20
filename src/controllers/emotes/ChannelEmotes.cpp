#include "controllers/emotes/ChannelEmotes.hpp"

#include "controllers/emotes/EmoteController.hpp"
#include "controllers/emotes/EmoteProvider.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "util/WeakPtrHelpers.hpp"

#include <QStringBuilder>

namespace chatterino {

using namespace Qt::Literals;

ChannelEmotes::ChannelEmotes(TwitchChannel *channel)
    : channel(channel)
{
}

void ChannelEmotes::initialize(const EmoteController &controller)
{
    for (const auto &provider : controller.getProviders())
    {
        if (!provider->supportsChannel(this->channel))
        {
            continue;
        }

        this->providerData_.emplace_back(ProviderData{
            .provider = provider,
            .id = provider->id(),
            .emotes = EMPTY_EMOTE_MAP,
        });
        this->signalHolder.addConnection(provider->channelEmotesEnabled.connect(
            [this, provider](bool enabled) {
                auto *item = this->dataForProvider(provider);
                if (!item)
                {
                    return;
                }

                if (!enabled)
                {
                    item->emotes = EMPTY_EMOTE_MAP;
                    return;
                }

                ChannelEmotes::refreshProvider(*item, this->channel, false);
            }));
    }
    this->sort();
}

EmotePtr ChannelEmotes::resolve(const EmoteName &query) const
{
    for (const auto &data : this->providerData_)
    {
        assert(data.emotes);
        auto it = data.emotes->find(query);
        if (it != data.emotes->end())
        {
            return it->second;
        }
    }
    return nullptr;
}

void ChannelEmotes::refresh(bool manualRefresh)
{
    for (const auto &item : this->providerData_)
    {
        ChannelEmotes::refreshProvider(item, this->channel, manualRefresh);
    }
}

ChannelEmotes::ProviderData *ChannelEmotes::dataForProvider(
    const std::weak_ptr<EmoteProvider> &provider)
{
    for (auto &item : this->providerData_)
    {
        if (weakOwnerEquals(item.provider, provider))
        {
            return &item;
        }
    }
    return nullptr;
}

void ChannelEmotes::refreshProvider(const ProviderData &data,
                                    TwitchChannel *channel, bool manualRefresh)
{
    auto provider = data.provider.lock();
    if (!provider || !channel)
    {
        return;
    }
    auto channelSP = channel->sharedTwitchChannel();

    provider->loadChannelEmotes(
        channelSP,
        [channelWP = std::weak_ptr(channelSP), provider = data.provider,
         manualRefresh,
         hasPreemptive = false](ExpectedStr<EmoteLoadResult> result) mutable {
            auto chan = channelWP.lock();
            if (!chan)
            {
                return;  // channel expired
            }

            auto &emotes = chan->channelEmotes();
            auto *item = emotes.dataForProvider(provider);
            if (!item)
            {
                return;
            }
            auto itemProvider = item->provider.lock();
            if (!itemProvider)
            {
                return;
            }

            QString message;
            if (result)
            {
                item->emotes = std::move(result->emotes);
                if (!item->emotes)
                {
                    item->emotes = EMPTY_EMOTE_MAP;
                }
                if (result->isPreemptiveCached)
                {
                    hasPreemptive = true;
                    return;
                }

                if (manualRefresh)
                {
                    if (item->emotes->empty())
                    {
                        message = u"This channel has no " %
                                  itemProvider->name() % " emotes.";
                    }
                    else
                    {
                        message =
                            itemProvider->name() % u" channel emotes reloaded.";
                    }
                }
                // else: initial refresh (no message)
            }
            else
            {
                message = u"Failed to refresh " % itemProvider->name() %
                          u" channel emotes: " % result.error();
                if (hasPreemptive)
                {
                    message += u". Using cached emotes."_s;
                }
            }

            if (!message.isEmpty())
            {
                chan->addSystemMessage(message);
            }
        },
        EmoteProvider::LoadChannelArgs{
            .manualRefresh = manualRefresh,
        });
}

void ChannelEmotes::sort()
{
    std::ranges::sort(this->providerData_, {}, [](const auto &item) {
        auto provider = item.provider.lock();
        if (!provider)
        {
            return std::numeric_limits<uint32_t>::max();
        }
        return provider->priority();
    });
}

}  // namespace chatterino
