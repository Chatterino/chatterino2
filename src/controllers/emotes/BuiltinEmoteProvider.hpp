#pragma once

#include "common/ChatterinoSetting.hpp"
#include "controllers/emotes/EmoteProvider.hpp"
#include "util/FunctionRef.hpp"

#include <memory>

class QJsonValue;

namespace chatterino {

class TwitchChannel;

class EmoteHolder;

class BuiltinEmoteProvider
    : public EmoteProvider,
      public std::enable_shared_from_this<BuiltinEmoteProvider>
{
public:
    static constexpr uint32_t BTTV_PRIORITY = 10;
    static constexpr uint32_t FFZ_PRIORITY = 20;
    static constexpr uint32_t SEVENTV_PRIORITY = 30;

    BuiltinEmoteProvider(BoolSetting *globalSetting, QString globalUrl,
                         BoolSetting *channelSetting, QString name, QString id,
                         uint32_t priority);

    void initialize() override;

    void reloadGlobalEmotes(
        std::function<void(ExpectedStr<void>)> onDone) override;

    void loadChannelEmotes(
        const std::shared_ptr<EmoteChannel> &channel,
        std::function<void(ExpectedStr<EmoteLoadResult>)> onDone,
        LoadChannelArgs args) override;

    bool supportsChannel(EmoteChannel *channel) override;

    bool hasChannelEmotes() const override;
    bool hasGlobalEmotes() const override;

protected:
    virtual std::optional<EmoteMap> parseChannelEmotes(
        TwitchChannel &twitch, const QJsonValue &json) = 0;
    virtual std::optional<EmoteMap> parseGlobalEmotes(
        const QJsonValue &json) = 0;

    virtual QString channelEmotesUrl(const TwitchChannel &twitch) const = 0;

    // convenience functions for live updates

    /// Add an emote to a channel
    ///
    /// This will copy the underlying emote map and update the one in `holder`.
    ///
    /// @returns True if an update took place
    bool addChannelEmote(EmoteHolder &holder, EmotePtr emote);

    /// Update an emote by its ID (with an optional hint)
    ///
    /// @param holder The channel emotes
    /// @param id The ID of the emote to update
    /// @param nameHint A hint of the previous emote name (used to speed up
    ///                 lookup). Unused if empty.
    /// @param createUpdatedEmote A callback to create the new emote. This is
    ///                           only called if the emote is found. This can
    ///                           return an empty emote pointer if nothing
    ///                           changed.
    /// @returns `(old emote, new emote)` if the emote was found. Otherwise,
    ///          `std::nullopt`.
    std::optional<std::pair<EmotePtr, EmotePtr>> updateChannelEmote(
        EmoteHolder &holder, const QString &id, const QString &nameHint,
        FunctionRef<EmotePtr(const EmotePtr &)> createUpdatedEmote);

    /// Remove an emote by its ID (with an optional hint)
    ///
    /// @returns The removed emote if it existed. Otherwise, an empty shared
    ///          pointer.
    EmotePtr removeChannelEmote(EmoteHolder &holder, const QString &id,
                                const QString &nameHint);

private:
    BoolSetting *globalSetting;
    QString globalUrl;

    BoolSetting *channelSetting;

    pajlada::Signals::SignalHolder signals;
};

}  // namespace chatterino
