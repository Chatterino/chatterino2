#include "providers/bttv/BttvBadges.hpp"

#include "messages/Emote.hpp"
#include "messages/Image.hpp"

namespace chatterino {

QString BttvBadges::idForBadge(const QJsonObject &badgeJson) const
{
    return badgeJson["url"].toString();
}

EmotePtr BttvBadges::createBadge(const QString &id,
                                 const QJsonObject & /* badgeJson */) const
{
    auto emote = Emote{
        .name = EmoteName{},
        .images = ImageSet(Image::fromUrl(Url{id}, 18.0 / 72.0)),
        .tooltip = Tooltip{"BTTV Pro"},
        .homePage = Url{},
        .id = EmoteId{id},
    };

    return std::make_shared<const Emote>(std::move(emote));
}

}  // namespace chatterino
