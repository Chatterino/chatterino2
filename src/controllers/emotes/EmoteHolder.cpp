#include "controllers/emotes/EmoteHolder.hpp"

#include "common/Channel.hpp"
#include "controllers/emotes/EmoteController.hpp"
#include "controllers/emotes/EmoteProvider.hpp"
#include "util/Functional.hpp"
#include "util/WeakPtrHelpers.hpp"

#include <QStringBuilder>

namespace chatterino {

using namespace Qt::Literals;
using namespace functional;

EmoteHolder::EmoteHolder(Channel *channel)
    : channel(channel)
{
}

void EmoteHolder::initialize(const EmoteController &controller)
{
    for (const auto &provider : controller.getProviders())
    {
        if (!provider->supportsChannel(channel))
        {
            continue;
        }

        this->items_.emplace_back(Item{
            .provider = provider,
            .id = provider->id(),
            .emotes = EMPTY_EMOTE_MAP,
        });
        this->signalHolder.addConnection(provider->channelEmotesEnabled.connect(
            [this, provider](bool enabled) {
                auto *item = this->itemByProvider(provider);
                if (!item)
                {
                    return;
                }

                if (!enabled)
                {
                    item->emotes = EMPTY_EMOTE_MAP;
                    return;
                }

                EmoteHolder::refreshItem(*item, this->channel, false);
            }));
    }
    this->sort();
}

EmotePtr EmoteHolder::resolve(const EmoteName &query) const
{
    for (const auto &item : this->items_)
    {
        assert(item.emotes);
        auto it = item.emotes->find(query);
        if (it != item.emotes->end())
        {
            return it->second;
        }
    }
    return nullptr;
}

void EmoteHolder::refresh(bool manualRefresh)
{
    for (const auto &item : this->items_)
    {
        EmoteHolder::refreshItem(item, this->channel, manualRefresh);
    }
}

EmoteHolder::Item *EmoteHolder::itemByProvider(
    const std::weak_ptr<EmoteProvider> &provider)
{
    for (auto &item : this->items_)
    {
        if (weakOwnerEquals(item.provider, provider))
        {
            return &item;
        }
    }
    return nullptr;
}

void EmoteHolder::refreshItem(const Item &item, Channel *channel,
                              bool manualRefresh)
{
    auto provider = item.provider.lock();
    if (!provider || !channel)
    {
        return;
    }
    auto channelSP = channel->shared_from_this();
    if (!channelSP)
    {
        assert(false && "Channel without shared_from_this");
        return;
    }

    provider->loadChannelEmotes(
        channelSP,
        weakGuarded(
            [provider = item.provider, manualRefresh, hasPreemptive = false](
                const std::shared_ptr<Channel> &chan,
                ExpectedStr<EmoteLoadResult> result) mutable {
                auto *emotes = chan->emotes();
                if (!emotes)
                {
                    return;
                }

                auto *item = emotes->itemByProvider(provider);
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
                            message = itemProvider->name() %
                                      u" channel emotes reloaded.";
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
            channelSP),
        EmoteProvider::LoadChannelArgs{.manualRefresh = manualRefresh});
}

void EmoteHolder::sort()
{
    std::ranges::sort(this->items_, {}, [](const auto &item) {
        auto provider = item.provider.lock();
        if (!provider)
        {
            return std::numeric_limits<uint32_t>::max();
        }
        return provider->priority();
    });
}

}  // namespace chatterino
