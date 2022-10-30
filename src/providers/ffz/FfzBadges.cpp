#include "FfzBadges.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QThread>
#include <QUrl>
#include <map>
#include <shared_mutex>
#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "messages/Emote.hpp"

namespace chatterino {

void FfzBadges::initialize(Settings &settings, Paths &paths)
{
    this->load();
}

std::vector<FfzBadges::Badge> FfzBadges::getUserBadges(const UserId &id)
{
    std::vector<Badge> badges;

    std::shared_lock lock(this->mutex_);

    auto it = this->userBadges.find(id.string);
    if (it != this->userBadges.end())
    {
        for (const auto &badgeID : it->second)
        {
            if (auto badge = this->getBadge(badgeID); badge)
            {
                badges.emplace_back(*badge);
            }
        }
    }

    return badges;
}

boost::optional<FfzBadges::Badge> FfzBadges::getBadge(const int badgeID)
{
    auto it = this->badges.find(badgeID);
    if (it != this->badges.end())
    {
        return it->second;
    }

    return boost::none;
}

void FfzBadges::load()
{
    static QUrl url("https://api.frankerfacez.com/v1/badges/ids");

    NetworkRequest(url)
        .onSuccess([this](auto result) -> Outcome {
            std::unique_lock lock(this->mutex_);

            auto jsonRoot = result.parseJson();
            for (const auto &jsonBadge_ : jsonRoot.value("badges").toArray())
            {
                auto jsonBadge = jsonBadge_.toObject();
                auto jsonUrls = jsonBadge.value("urls").toObject();

                auto emote = Emote{
                    EmoteName{},
                    ImageSet{
                        Url{QString("https:") + jsonUrls.value("1").toString()},
                        Url{QString("https:") + jsonUrls.value("2").toString()},
                        Url{QString("https:") +
                            jsonUrls.value("4").toString()}},
                    Tooltip{jsonBadge.value("title").toString()}, Url{}};

                Badge badge;

                int badgeID = jsonBadge.value("id").toInt();

                this->badges[badgeID] = Badge{
                    std::make_shared<const Emote>(std::move(emote)),
                    QColor(jsonBadge.value("color").toString()),
                };

                // Find users with this badge
                auto badgeIDString = QString::number(badgeID);
                for (const auto &user : jsonRoot.value("users")
                                            .toObject()
                                            .value(badgeIDString)
                                            .toArray())
                {
                    auto userIDString = QString::number(user.toInt());

                    auto [userBadges, created] = this->userBadges.emplace(
                        std::make_pair<QString, std::vector<int>>(
                            std::move(userIDString), {badgeID}));
                    if (!created)
                    {
                        // User already had a badge assigned
                        userBadges->second.push_back(badgeID);
                    }
                }
            }

            return Success;
        })
        .execute();
}

}  // namespace chatterino
