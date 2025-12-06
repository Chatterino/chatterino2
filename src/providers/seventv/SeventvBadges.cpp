#include "providers/seventv/SeventvBadges.hpp"

#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "providers/seventv/SeventvEmotes.hpp"

namespace chatterino {

QString SeventvBadges::idForBadge(const QJsonObject &badgeJson) const
{
    return badgeJson["id"].toString();
}

EmotePtr SeventvBadges::createBadge(const QString &id,
                                    const QJsonObject &badgeJson) const
{
    auto emote = Emote{
        .name = EmoteName{},
        .images = SeventvEmotes::createImageSet(badgeJson, true),
        .tooltip = Tooltip{badgeJson["tooltip"].toString()},
        .homePage = Url{},
        .id = EmoteId{id},
    };

    if (emote.images.getImage1()->isEmpty())
    {
        return nullptr;  // Bad images
    }

    return std::make_shared<const Emote>(std::move(emote));
}

}  // namespace chatterino
