#include "providers/chatterino/ChatterinoBadges.hpp"

#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>

namespace chatterino {

ChatterinoBadges::ChatterinoBadges()
{
    this->loadChatterinoBadges();
}

std::optional<EmotePtr> ChatterinoBadges::getBadge(const UserId &id)
{
    std::shared_lock lock(this->mutex_);

    auto it = badgeMap.find(id.string);
    if (it != badgeMap.end())
    {
        return emotes[it->second];
    }
    return std::nullopt;
}

void ChatterinoBadges::loadChatterinoBadges()
{
    static QUrl url("https://api.chatterino.com/badges");

    NetworkRequest(url)
        .concurrent()
        .onSuccess([this](auto result) {
            auto jsonRoot = result.parseJson();

            std::unique_lock lock(this->mutex_);

            int index = 0;
            for (const auto &jsonBadgeValue :
                 jsonRoot.value("badges").toArray())
            {
                auto jsonBadge = jsonBadgeValue.toObject();
                // The sizes for the images are only an estimation, there might
                // be badges with different sizes.
                constexpr QSize baseSize(18, 18);
                auto emote = Emote{
                    .name = EmoteName{},
                    .images =
                        ImageSet{
                            Image::fromUrl(
                                Url{jsonBadge.value("image1").toString()}, 1.0,
                                baseSize),
                            Image::fromUrl(
                                Url{jsonBadge.value("image2").toString()}, 0.5,
                                baseSize * 2),
                            Image::fromUrl(
                                Url{jsonBadge.value("image3").toString()}, 0.25,
                                baseSize * 4),
                        },
                    .tooltip = Tooltip{jsonBadge.value("tooltip").toString()},
                    .homePage = Url{},
                };

                emotes.push_back(
                    std::make_shared<const Emote>(std::move(emote)));

                for (const auto &user : jsonBadge.value("users").toArray())
                {
                    badgeMap[user.toString()] = index;
                }
                ++index;
            }
        })
        .execute();
}

}  // namespace chatterino
