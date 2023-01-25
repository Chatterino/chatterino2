#include "DankerinoBadges.hpp"

#include "common/NetworkRequest.hpp"
#include "common/NetworkResult.hpp"
#include "common/Outcome.hpp"
#include "messages/Emote.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QThread>
#include <QUrl>

#include <map>
#include <memory>

namespace chatterino {
void DankerinoBadges::initialize(Settings &settings, Paths &paths)
{
    this->loadDankerinoBadges();
}

DankerinoBadges::DankerinoBadges()
{
}

boost::optional<EmotePtr> DankerinoBadges::getBadge(const UserId &id)
{
    auto it = badgeMap.find(id.string);
    if (it != badgeMap.end())
    {
        return emotes[it->second];
    }
    return boost::none;
}

void DankerinoBadges::loadDankerinoBadges()
{
    static QUrl url("https://raw.githubusercontent.com/Mm2PL/chatterino2/"
                    "dankerino/badges.json");

    NetworkRequest(url)
        .onSuccess([this](const NetworkResult &result) -> Outcome {
            auto jsonRoot = result.parseJson();
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
