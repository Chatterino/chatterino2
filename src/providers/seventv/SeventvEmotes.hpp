#pragma once

#include "boost/optional.hpp"
#include "common/Aliases.hpp"
#include "common/Atomic.hpp"
#include "providers/twitch/TwitchChannel.hpp"

#include <memory>

namespace chatterino {

// https://github.com/SevenTV/API/blob/a84e884b5590dbb5d91a5c6b3548afabb228f385/data/model/emote-set.model.go#L29-L36
enum class SeventvActiveEmoteFlag : int64_t {
    None = 0LL,

    // Emote is zero-width
    ZeroWidth = (1LL << 0),

    // Overrides Twitch Global emotes with the same name
    OverrideTwitchGlobal = (1 << 16),
    // Overrides Twitch Subscriber emotes with the same name
    OverrideTwitchSubscriber = (1 << 17),
    // Overrides BetterTTV emotes with the same name
    OverrideBetterTTV = (1 << 18),
};

// https://github.com/SevenTV/API/blob/a84e884b5590dbb5d91a5c6b3548afabb228f385/data/model/emote.model.go#L57-L70
enum class SeventvEmoteFlag : int64_t {
    None = 0LL,
    // The emote is private and can only be accessed by its owner, editors and moderators
    Private = 1 << 0,
    // The emote was verified to be an original creation by the uploader
    Authentic = (1LL << 1),
    // The emote is recommended to be enabled as Zero-Width
    ZeroWidth = (1LL << 8),

    // Content Flags

    // Sexually Suggesive
    ContentSexual = (1LL << 16),
    // Rapid flashing
    ContentEpilepsy = (1LL << 17),
    // Edgy or distasteful, may be offensive to some users
    ContentEdgy = (1 << 18),
    // Not allowed specifically on the Twitch platform
    ContentTwitchDisallowed = (1LL << 24),
};

using SeventvActiveEmoteFlags = FlagsEnum<SeventvActiveEmoteFlag>;
using SeventvEmoteFlags = FlagsEnum<SeventvEmoteFlag>;

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;
class EmoteMap;

class SeventvEmotes final
{
public:
    SeventvEmotes();

    std::shared_ptr<const EmoteMap> globalEmotes() const;
    boost::optional<EmotePtr> globalEmote(const EmoteName &name) const;
    void loadGlobalEmotes();
    static void loadChannelEmotes(const std::weak_ptr<Channel> &channel,
                                  const QString &channelId,
                                  std::function<void(EmoteMap &&)> callback,
                                  bool manualRefresh);

private:
    Atomic<std::shared_ptr<const EmoteMap>> global_;
};

}  // namespace chatterino
