#include "providers/chatterino/ChatterinoBadges.hpp"

#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "messages/Emote.hpp"

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
                auto emote = Emote{
                    .name = EmoteName{},
                    .images =
                        ImageSet{Url{jsonBadge.value("image1").toString()},
                                 Url{jsonBadge.value("image2").toString()},
                                 Url{jsonBadge.value("image3").toString()}},
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
