#pragma once

#include "controllers/emotes/BuiltinEmoteProvider.hpp"

#include <QJsonArray>
#include <QString>

#include <functional>
#include <memory>
#include <optional>

namespace chatterino {

class ImageSet;
namespace seventv::eventapi {
struct EmoteAddDispatch;
struct EmoteUpdateDispatch;
struct EmoteRemoveDispatch;
}  // namespace seventv::eventapi

namespace seventv::detail {

std::optional<Emote> parseEmote(const QJsonObject &activeEmote, bool isGlobal);

}  // namespace seventv::detail

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;
class EmoteMap;

class SeventvEmoteProvider final : public BuiltinEmoteProvider
{
public:
    SeventvEmoteProvider();

    static std::shared_ptr<SeventvEmoteProvider> instance();
    std::shared_ptr<SeventvEmoteProvider> shared();

    void initialize() override;

    EmotePtr addEmote(TwitchChannel *channel,
                      const seventv::eventapi::EmoteAddDispatch &message);
    std::optional<std::pair<EmotePtr, EmotePtr>> updateEmote(
        TwitchChannel *channel,
        const seventv::eventapi::EmoteUpdateDispatch &message);
    EmotePtr removeEmote(TwitchChannel *channel,
                         const seventv::eventapi::EmoteRemoveDispatch &message);

    struct EmoteSetData {
        QString name;
    };
    void applyEmoteSet(
        TwitchChannel *channel, const QString &emoteSetID,
        const std::function<void(ExpectedStr<EmoteSetData>)> &onDone);

    /**
     * Creates an image set from a 7TV emote or badge.
     *
     * @param emoteData { host: { files: [], url } }
     * @param useStatic use static version if possible
     */
    static ImageSet createImageSet(const QJsonObject &emoteData,
                                   bool useStatic);

protected:
    std::optional<EmoteMap> parseChannelEmotes(TwitchChannel &twitch,
                                               const QJsonValue &json) override;
    std::optional<EmoteMap> parseGlobalEmotes(const QJsonValue &json) override;

    QString channelEmotesUrl(const TwitchChannel &twitch) const override;

private:
    EmoteMap parseEmotes(const QJsonArray &emoteSetEmotes, bool isGlobal);

    static std::weak_ptr<SeventvEmoteProvider> INSTANCE;
};

}  // namespace chatterino
