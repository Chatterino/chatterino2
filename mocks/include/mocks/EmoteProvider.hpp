#pragma once

#include "controllers/emotes/EmoteChannel.hpp"
#include "controllers/emotes/EmoteProvider.hpp"

#include <QStringBuilder>

namespace chatterino::mock {

class EmoteProvider : public chatterino::EmoteProvider
{
public:
    EmoteProvider(QString name, QString id, uint32_t priority)
        : chatterino::EmoteProvider(std::move(name), std::move(id), priority)
    {
    }

    void setGlobalEmotes(EmoteMapPtr map)
    {
        this->globalEmotes_ = std::move(map);
    }

    void setChannel(const QString &name, EmoteMapPtr map)
    {
        this->channels[name] = std::move(map);
    }

    void setChannelFallback(EmoteMapPtr map)
    {
        this->channelFallback = std::move(map);
    }

    void initialize() override
    {
    }

    void reloadGlobalEmotes(
        std::function<void(ExpectedStr<void>)> onDone) override
    {
        onDone({});
    }

    void loadChannelEmotes(
        const std::shared_ptr<EmoteChannel> &channel,
        std::function<void(ExpectedStr<EmoteLoadResult>)> onDone,
        LoadChannelArgs /* args */) override
    {
        auto it = channels.find(channel->getName());
        if (it == this->channels.end())
        {
            if (this->channelFallback)
            {
                onDone(EmoteLoadResult{.emotes = this->channelFallback});
            }
            else
            {
                onDone(makeUnexpected("No emotes for this channel"));
            }
        }
        else
        {
            onDone(EmoteLoadResult{.emotes = it->second});
        }
    }

    bool supportsChannel(EmoteChannel * /*channel*/) override
    {
        return true;
    }

    bool hasChannelEmotes() const override
    {
        return true;
    }

    bool hasGlobalEmotes() const override
    {
        return true;
    }

    QString emoteUrl(const Emote &emote) const override
    {
        return u"https://chatterino.com/" % emote.name.string;
    }

    std::unordered_map<QString, EmoteMapPtr> channels;
    EmoteMapPtr channelFallback;
};

}  // namespace chatterino::mock
