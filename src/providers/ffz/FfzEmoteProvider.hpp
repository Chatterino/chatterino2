#pragma once

#include "controllers/emotes/BuiltinEmoteProvider.hpp"
#include "util/QStringHash.hpp"

#include <boost/unordered/unordered_flat_map.hpp>
#include <pajlada/signals/scoped-connection.hpp>
#include <QJsonObject>
#include <QString>

#include <memory>
#include <optional>
#include <vector>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;
class EmoteMap;
class Channel;

/// Maps a Twitch User ID to a list of badge IDs
using FfzChannelBadgeMap =
    boost::unordered::unordered_flat_map<QString, std::vector<int>>;

class FfzEmoteProvider : public BuiltinEmoteProvider
{
public:
    FfzEmoteProvider();

    QString emoteUrl(const Emote &emote) const override;

protected:
    std::optional<EmoteMap> parseChannelEmotes(TwitchChannel &twitch,
                                               const QJsonValue &json) override;
    std::optional<EmoteMap> parseGlobalEmotes(const QJsonValue &json) override;

    QString channelEmotesUrl(const TwitchChannel &twitch) const override;

private:
    void parseEmoteSetInto(const QJsonObject &emoteSet, const QString &kind,
                           EmoteMap &map);
};

}  // namespace chatterino
