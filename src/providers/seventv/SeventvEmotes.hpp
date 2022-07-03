#pragma once

#include "boost/optional.hpp"
#include "common/Aliases.hpp"
#include "common/Atomic.hpp"
#include "providers/twitch/TwitchChannel.hpp"

#include <memory>

namespace chatterino {

// https://github.com/SevenTV/ServerGo/blob/dfe867f991e8cfd7a79d93b9bec681216c32abdb/src/mongo/datastructure/datastructure.go#L56-L67
enum class SeventvEmoteVisibilityFlag : int64_t {
    None = 0LL,

    Private = (1LL << 0),
    Global = (1LL << 1),
    Unlisted = (1LL << 2),

    OverrideBttv = (1LL << 3),
    OverrideFfz = (1LL << 4),
    OverrideTwitchGlobal = (1LL << 5),
    OverrideTwitchSubscriber = (1LL << 6),

    ZeroWidth = (1LL << 7),
};

using SeventvEmoteVisibilityFlags = FlagsEnum<SeventvEmoteVisibilityFlag>;

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;
class EmoteMap;

class SeventvEmotes final
{
    static constexpr const char *apiUrlGQL = "https://api.7tv.app/v2/gql";

public:
    SeventvEmotes();

    std::shared_ptr<const EmoteMap> emotes() const;
    boost::optional<EmotePtr> emote(const EmoteName &name) const;
    void loadEmotes();
    static boost::optional<EmotePtr> addEmote(
        Atomic<std::shared_ptr<const EmoteMap>> &map,
        const QJsonValue &emoteJson);
    static boost::optional<EmotePtr> updateEmote(
        Atomic<std::shared_ptr<const EmoteMap>> &map, QString *emoteBaseName,
        const QJsonValue &emoteJson);
    static bool removeEmote(Atomic<std::shared_ptr<const EmoteMap>> &map,
                            const QString &emoteName);
    static void loadChannel(std::weak_ptr<Channel> channel,
                            const QString &channelId,
                            std::function<void(EmoteMap &&)> callback,
                            bool manualRefresh);

private:
    Atomic<std::shared_ptr<const EmoteMap>> global_;
};

}  // namespace chatterino
