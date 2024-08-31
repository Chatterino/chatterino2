#pragma once

#include "common/Aliases.hpp"
#include "common/Atomic.hpp"
#include "util/QStringHash.hpp"

#include <boost/unordered/unordered_flat_map.hpp>
#include <pajlada/signals/scoped-connection.hpp>
#include <QJsonObject>
#include <QString>

#include <functional>
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

namespace ffz::detail {

    EmoteMap parseChannelEmotes(const QJsonObject &jsonRoot);

    /**
     * Parse the `user_badge_ids` into a map of User IDs -> Badge IDs
     */
    FfzChannelBadgeMap parseChannelBadges(const QJsonObject &badgeRoot);

}  // namespace ffz::detail

class FfzEmotes final
{
public:
    FfzEmotes();

    std::shared_ptr<const EmoteMap> emotes() const;
    std::optional<EmotePtr> emote(const EmoteName &name) const;
    void loadEmotes();
    void setEmotes(std::shared_ptr<const EmoteMap> emotes);
    static void loadChannel(
        std::weak_ptr<Channel> channel, const QString &channelId,
        std::function<void(EmoteMap &&)> emoteCallback,
        std::function<void(std::optional<EmotePtr>)> modBadgeCallback,
        std::function<void(std::optional<EmotePtr>)> vipBadgeCallback,
        std::function<void(FfzChannelBadgeMap &&)> channelBadgesCallback,
        bool manualRefresh);

private:
    Atomic<std::shared_ptr<const EmoteMap>> global_;

    std::vector<std::unique_ptr<pajlada::Signals::ScopedConnection>>
        managedConnections;
};

}  // namespace chatterino
