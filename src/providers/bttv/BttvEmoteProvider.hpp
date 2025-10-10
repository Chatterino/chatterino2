#pragma once

#include "controllers/emotes/BuiltinEmoteProvider.hpp"

#include <QJsonValue>

#include <optional>

namespace chatterino {

struct BttvLiveUpdateEmoteUpdateAddMessage;
struct BttvLiveUpdateEmoteRemoveMessage;

class BttvEmoteProvider final : public BuiltinEmoteProvider
{
public:
    BttvEmoteProvider();

    static std::shared_ptr<BttvEmoteProvider> instance();

    void initialize() override;

    EmotePtr addEmote(TwitchChannel *channel,
                      const BttvLiveUpdateEmoteUpdateAddMessage &message);
    std::optional<std::pair<EmotePtr, EmotePtr>> updateEmote(
        TwitchChannel *channel,
        const BttvLiveUpdateEmoteUpdateAddMessage &message);
    EmotePtr removeEmote(TwitchChannel *channel,
                         const BttvLiveUpdateEmoteRemoveMessage &message);

protected:
    std::optional<EmoteMap> parseChannelEmotes(TwitchChannel &twitch,
                                               const QJsonValue &json) override;
    std::optional<EmoteMap> parseGlobalEmotes(const QJsonValue &json) override;

    QString channelEmotesUrl(const TwitchChannel &twitch) const override;

private:
    static std::weak_ptr<BttvEmoteProvider> INSTANCE;
};

}  // namespace chatterino
