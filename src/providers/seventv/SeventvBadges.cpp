#include "SeventvBadges.hpp"

#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "messages/Emote.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QThread>
#include <QUrl>
#include <QUrlQuery>

#include <map>

namespace chatterino {
void SeventvBadges::initialize(Settings &settings, Paths &paths)
{
    this->loadSeventvBadges();
}

SeventvBadges::SeventvBadges()
{
}

boost::optional<EmotePtr> SeventvBadges::getBadge(const UserId &id)
{
    auto it = badgeMap.find(id.string);
    if (it != badgeMap.end())
    {
        return emotes[it->second];
    }
    return boost::none;
}

void SeventvBadges::loadSeventvBadges()
{
    static QUrl url("https://api.7tv.app/v2/badges");

    static QUrlQuery urlQuery;
    // valid user_identifier values: "object_id", "twitch_id", "login"
    urlQuery.addQueryItem("user_identifier", "twitch_id");

    url.setQuery(urlQuery);

    NetworkRequest(url)
        .onSuccess([this](NetworkResult result) -> Outcome {
            auto root = result.parseJson();

            int index = 0;
            for (const auto &jsonBadge_ : root.value("badges").toArray())
            {
                auto badge = jsonBadge_.toObject();
                auto urls = badge.value("urls").toArray();
                auto emote =
                    Emote{EmoteName{},
                          ImageSet{Url{urls.at(0).toArray().at(1).toString()},
                                   Url{urls.at(1).toArray().at(1).toString()},
                                   Url{urls.at(2).toArray().at(1).toString()}},
                          Tooltip{badge.value("tooltip").toString()}, Url{}};

                emotes.push_back(
                    std::make_shared<const Emote>(std::move(emote)));

                for (const auto &user : badge.value("users").toArray())
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
