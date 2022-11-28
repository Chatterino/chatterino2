#include "ChatterinoBadges.hpp"

#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "messages/Emote.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QThread>
#include <QUrl>

namespace chatterino {
void ChatterinoBadges::initialize(Settings &settings, Paths &paths)
{
    this->loadChatterinoBadges();
}

ChatterinoBadges::ChatterinoBadges()
{
}

boost::optional<EmotePtr> ChatterinoBadges::getBadge(const UserId &id)
{
    std::shared_lock lock(this->mutex_);

    auto it = badgeMap.find(id.string);
    if (it != badgeMap.end())
    {
        return emotes[it->second];
    }
    return boost::none;
}

void ChatterinoBadges::loadChatterinoBadges()
{
    static QUrl url("https://api.chatterino.com/badges");

    NetworkRequest(url)
        .concurrent()
        .onSuccess([this](auto result) -> Outcome {
            auto jsonRoot = result.parseJson();

            std::unique_lock lock(this->mutex_);

            int index = 0;
            for (const auto &jsonBadge_ : jsonRoot.value("badges").toArray())
            {
                auto jsonBadge = jsonBadge_.toObject();
                auto emote = Emote{
                    EmoteName{},
                    ImageSet{Url{jsonBadge.value("image1").toString()},
                             Url{jsonBadge.value("image2").toString()},
                             Url{jsonBadge.value("image3").toString()}},
                    Tooltip{jsonBadge.value("tooltip").toString()}, Url{}};

                emotes.push_back(
                    std::make_shared<const Emote>(std::move(emote)));

                for (const auto &user : jsonBadge.value("users").toArray())
                {
                    badgeMap[user.toString()] = index;
                }
                ++index;
            }

            return Success;
        })
        .execute();
}

}  // namespace chatterino
