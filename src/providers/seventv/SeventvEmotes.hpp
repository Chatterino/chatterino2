#pragma once

#include "common/Aliases.hpp"
#include "common/Atomic.hpp"
#include "common/FlagsEnum.hpp"

#include <pajlada/signals/scoped-connection.hpp>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace chatterino {

class ImageSet;
class Channel;
namespace seventv::eventapi {
    struct EmoteAddDispatch;
    struct EmoteUpdateDispatch;
    struct EmoteRemoveDispatch;
}  // namespace seventv::eventapi

// https://github.com/SevenTV/API/blob/a84e884b5590dbb5d91a5c6b3548afabb228f385/data/model/emote-set.model.go#L29-L36
enum class SeventvActiveEmoteFlag : std::int64_t {
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

enum class SeventvEmoteSetKind : uint8_t {
    Global,
    Personal,
    Channel,
};

enum class SeventvEmoteSetFlag : uint32_t {
    Immutable = (1 << 0),
    Privileged = (1 << 1),
    Personal = (1 << 2),
    Commercial = (1 << 3),
};
using SeventvEmoteSetFlags = FlagsEnum<SeventvEmoteSetFlag>;

namespace seventv::detail {

    EmoteMap parseEmotes(const QJsonArray &emoteSetEmotes, bool isGlobal);

}  // namespace seventv::detail

class SeventvEmotes final
{
public:
    struct ChannelInfo {
        QString userID;
        QString emoteSetID;
        size_t twitchConnectionIndex;
    };

    SeventvEmotes();

    std::shared_ptr<const EmoteMap> globalEmotes() const;
    std::optional<EmotePtr> globalEmote(const EmoteName &name) const;
    void loadGlobalEmotes();
    void setGlobalEmotes(std::shared_ptr<const EmoteMap> emotes);
    static void loadChannelEmotes(
        const std::weak_ptr<Channel> &channel, const QString &channelId,
        std::function<void(EmoteMap &&, ChannelInfo)> callback,
        bool manualRefresh);

    /**
     * Adds an emote to the `map` if it's valid.
     * This will _copy_ the emote map and
     * update the `Atomic`.
     *
     * @return The added emote if an emote was added.
     */
    static std::optional<EmotePtr> addEmote(
        Atomic<std::shared_ptr<const EmoteMap>> &map,
        const seventv::eventapi::EmoteAddDispatch &dispatch);

    /**
     * Updates an emote in this `map`.
     * This will _copy_ the emote map and
     * update the `Atomic`.
     *
     * @return The updated emote if any emote was updated.
     */
    static std::optional<EmotePtr> updateEmote(
        Atomic<std::shared_ptr<const EmoteMap>> &map,
        const seventv::eventapi::EmoteUpdateDispatch &dispatch);

    /**
     * Removes an emote from this `map`.
     * This will _copy_ the emote map and
     * update the `Atomic`.
     *
     * @return The removed emote if any emote was removed.
     */
    static std::optional<EmotePtr> removeEmote(
        Atomic<std::shared_ptr<const EmoteMap>> &map,
        const seventv::eventapi::EmoteRemoveDispatch &dispatch);

    /** Fetches an emote-set by its id */
    static void getEmoteSet(
        const QString &emoteSetId,
        std::function<void(EmoteMap &&, QString)> successCallback,
        std::function<void(QString)> errorCallback);

    /**
     * Creates an image set from a 7TV emote or badge.
     *
     * @param emoteData { host: { files: [], url } }
     * @param useStatic use static version if possible
     */
    static ImageSet createImageSet(const QJsonObject &emoteData,
                                   bool useStatic);

private:
    Atomic<std::shared_ptr<const EmoteMap>> global_;

    std::vector<std::unique_ptr<pajlada::Signals::ScopedConnection>>
        managedConnections;
};

}  // namespace chatterino
