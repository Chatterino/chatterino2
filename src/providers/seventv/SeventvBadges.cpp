#include "providers/seventv/SeventvBadges.hpp"

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
    std::shared_lock lock(this->mutex_);

    auto it = this->badgeMap_.find(id.string);
    if (it != this->badgeMap_.end())
    {
        return this->emotes_[it->second];
    }
    return boost::none;
}

void SeventvBadges::loadSeventvBadges()
{
    // Cosmetics will work differently in v3, until this is ready
    // we'll use this endpoint.
    static QUrl url("https://7tv.io/v2/cosmetics");

    static QUrlQuery urlQuery;
    // valid user_identifier values: "object_id", "twitch_id", "login"
    urlQuery.addQueryItem("user_identifier", "twitch_id");

    url.setQuery(urlQuery);

    NetworkRequest(url)
        .onSuccess([this](const NetworkResult &result) -> Outcome {
            auto root = result.parseJson();

            std::shared_lock lock(this->mutex_);

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

                this->emotes_.push_back(
                    std::make_shared<const Emote>(std::move(emote)));

                for (const auto &user : badge.value("users").toArray())
                {
                    this->badgeMap_[user.toString()] = index;
                }
                ++index;
            }

            return Success;
        })
        .execute();
}

}  // namespace chatterino
