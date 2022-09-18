#pragma once

#include "boost/optional.hpp"
#include "common/Aliases.hpp"
#include "common/Atomic.hpp"
#include "providers/twitch/TwitchChannel.hpp"

#include <memory>

namespace chatterino {

// TODO(nerix): update these links when v3 is announced
//              add docs for each enum variant

// https://github.com/SevenTV/API/blob/8fbfc702a3fe0ada59f4b9593c65748d36ac7c0b/data/model/emote-set.model.go#L33-L38
enum class SeventvActiveEmoteFlag : int64_t {
    None = 0LL,

    ZeroWidth = (1LL << 0),

    OverrideTwitchGlobal = (1 << 16),
    OverrideTwitchSubscriber = (1 << 17),
    OverrideBetterTTV = (1 << 18),
};

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
    static void loadChannelEmotes(std::weak_ptr<Channel> channel,
                                  const QString &channelId,
                                  std::function<void(EmoteMap &&)> callback,
                                  bool manualRefresh);

private:
    Atomic<std::shared_ptr<const EmoteMap>> global_;
};

}  // namespace chatterino