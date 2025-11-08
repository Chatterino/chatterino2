#pragma once

#include "messages/Emote.hpp"
#include "util/Expected.hpp"

#include <pajlada/signals/signal.hpp>
#include <QString>

#include <memory>

namespace chatterino {

class Channel;

struct EmoteLoadResult {
    EmoteMapPtr emotes;
    bool isPreemptiveCached = false;
};

class EmoteProvider
{
public:
    struct LoadChannelArgs {
        bool manualRefresh = false;
    };

    EmoteProvider(QString name, QString id, uint32_t priority);
    virtual ~EmoteProvider();

    virtual void initialize() = 0;

    virtual void reloadGlobalEmotes(
        std::function<void(ExpectedStr<void>)> onDone) = 0;
    virtual void loadChannelEmotes(
        const std::shared_ptr<Channel> &channel,
        std::function<void(ExpectedStr<EmoteLoadResult>)> onDone,
        LoadChannelArgs args) = 0;

    virtual bool supportsChannel(Channel *channel) = 0;

    EmoteMapPtr globalEmotes() const;
    EmotePtr globalEmote(
        const EmoteName &name) const;  // XXX: should take a view

    const QString &name() const
    {
        return this->name_;
    }

    const QString &id() const
    {
        return this->id_;
    }

    uint32_t priority() const
    {
        return this->priority_;
    }

    virtual bool hasChannelEmotes() const = 0;
    virtual bool hasGlobalEmotes() const = 0;
    pajlada::Signals::Signal<bool> channelEmotesEnabled;

protected:
    EmotePtr createEmote(Emote &&emote);

    EmoteMapPtr globalEmotes_;

private:
    std::unordered_map<EmoteId, std::weak_ptr<const Emote>> cache;

    QString name_;
    QString id_;

    uint32_t priority_;
};

}  // namespace chatterino
