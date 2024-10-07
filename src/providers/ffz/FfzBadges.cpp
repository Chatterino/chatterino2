#include "providers/ffz/FfzBadges.hpp"

#include "Application.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "providers/ffz/FfzUtil.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QThread>
#include <QUrl>

namespace chatterino {

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

std::optional<FfzBadges::Badge> FfzBadges::getBadge(const int badgeID) const
{
    this->tgBadges.guard();
    auto it = this->badges.find(badgeID);
    if (it != this->badges.end())
    {
        return it->second;
    }

    return std::nullopt;
}

void FfzBadges::load()
{
    static QUrl url("https://api.frankerfacez.com/v1/badges/ids");

    NetworkRequest(url)
        .onSuccess([this](auto result) {
            std::unique_lock lock(this->mutex_);

            auto jsonRoot = result.parseJson();
            this->tgBadges.guard();
            for (const auto &jsonBadge_ : jsonRoot.value("badges").toArray())
            {
                auto jsonBadge = jsonBadge_.toObject();
                auto jsonUrls = jsonBadge.value("urls").toObject();
                QSize baseSize(jsonBadge["width"].toInt(18),
                               jsonBadge["height"].toInt(18));

                auto emote = Emote{
                    EmoteName{},
                    ImageSet{Image::fromUrl(
                                 parseFfzUrl(jsonUrls.value("1").toString()),
                                 1.0, baseSize),
                             Image::fromUrl(
                                 parseFfzUrl(jsonUrls.value("2").toString()),
                                 0.5, baseSize * 2),
                             Image::fromUrl(
                                 parseFfzUrl(jsonUrls.value("4").toString()),
                                 0.25, baseSize * 4)},
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
                        std::make_pair<QString, std::set<int>>(
                            std::move(userIDString), {badgeID}));
                    if (!created)
                    {
                        // User already had a badge assigned
                        userBadges->second.emplace(badgeID);
                    }
                }
            }
        })
        .execute();
}

void FfzBadges::registerBadge(int badgeID, Badge badge)
{
    assert(getApp()->isTest());

    std::unique_lock lock(this->mutex_);

    this->badges.emplace(badgeID, std::move(badge));
}

void FfzBadges::assignBadgeToUser(const UserId &userID, int badgeID)
{
    assert(getApp()->isTest());

    std::unique_lock lock(this->mutex_);

    auto it = this->userBadges.find(userID.string);
    if (it != this->userBadges.end())
    {
        it->second.emplace(badgeID);
    }
    else
    {
        this->userBadges.emplace(userID.string, std::set{badgeID});
    }
}

}  // namespace chatterino
