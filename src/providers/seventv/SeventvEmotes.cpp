#include "providers/seventv/SeventvEmotes.hpp"

#include <QJsonArray>
#include <QThread>

#include "common/Common.hpp"
#include "common/NetworkRequest.hpp"
#include "common/QLogging.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "messages/ImageSet.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchChannel.hpp"

namespace chatterino {
	namespace {
		const QString CHANNEL_HAS_NO_EMOTES(
			"This channel has no 7TV channel emotes."
		);
	}


SeventvEmotes::SeventvEmotes()
    : global_(std::make_shared<EmoteMap>())
{
}

std::shared_ptr<const EmoteMap> SeventvEmotes::emotes() const
{
	return this->global_.get();
}


boost::optional<EmotePtr> SeventvEmotes::emote(const EmoteName &name) const
{
    auto emotes = this->global_.get();
    auto it = emotes->find(name);

    if (it == emotes->end())
        return boost::none;
    return it->second;
}

void SeventvEmotes::loadEmotes()
{
    // TODO: Network request
}

void SeventvEmotes::loadChannel(std::weak_ptr<Channel> channel,
                             const QString &channelId,
                             const QString &channelDisplayName,
                             std::function<void(EmoteMap &&)> callback,
                             bool manualRefresh)
{
}

}
