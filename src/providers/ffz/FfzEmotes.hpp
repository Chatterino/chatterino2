#pragma once

#include "common/Aliases.hpp"
#include "common/Atomic.hpp"

#include <QJsonObject>

#include <memory>
#include <optional>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;
class EmoteMap;
class Channel;

namespace ffz::detail {

    EmoteMap parseChannelEmotes(const QJsonObject &jsonRoot);

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
        bool manualRefresh);

private:
    Atomic<std::shared_ptr<const EmoteMap>> global_;
};

}  // namespace chatterino
