#include "SeventvBadges.hpp"

#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "messages/Emote.hpp"

#include <QUrl>
#include <QUrlQuery>

#include <map>

namespace chatterino {
void SeventvBadges::initialize(Settings & /*settings*/, Paths & /*paths*/)
{
    this->loadSeventvBadges();
}

boost::optional<EmotePtr> SeventvBadges::getBadge(const UserId &id)
{
    auto it = badgeMap_.find(id.string);
    if (it != badgeMap_.end())
    {
        return emotes_[it->second];
    }
    return boost::none;
}

void SeventvBadges::loadSeventvBadges()
{
    // TODO(nerix): Migrate to v3.
    //              Status: waiting for cosmetics api.
    static QUrl url("https://api.7tv.app/v2/badges");

    static QUrlQuery urlQuery;
    // valid user_identifier values: "object_id", "twitch_id", "login"
    urlQuery.addQueryItem("user_identifier", "twitch_id");

    url.setQuery(urlQuery);

    NetworkRequest(url)
        .onSuccess([this](const NetworkResult &result) -> Outcome {
            auto root = result.parseJson();

            int index = 0;
            for (const auto &jsonBadge : root.value("badges").toArray())
            {
                auto badge = jsonBadge.toObject();
                auto urls = badge.value("urls").toArray();
                auto emote =
                    Emote{EmoteName{},
                          ImageSet{Url{urls.at(0).toArray().at(1).toString()},
                                   Url{urls.at(1).toArray().at(1).toString()},
                                   Url{urls.at(2).toArray().at(1).toString()}},
                          Tooltip{badge.value("tooltip").toString()}, Url{}};

                emotes_.push_back(
                    std::make_shared<const Emote>(std::move(emote)));

                for (const auto &user : badge.value("users").toArray())
                {
                    badgeMap_[user.toString()] = index;
                }
                ++index;
            }

            return Success;
        })
        .execute();
}

}  // namespace chatterino