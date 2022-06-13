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
    this->loadFfzBadges();
}

boost::optional<EmotePtr> FfzBadges::getBadge(const UserId &id, const int index)
{
    std::shared_lock lock(this->mutex_);

    auto it = this->badgeMap.find(id.string);
    if (it != this->badgeMap.end())
    {
        return this->badges[it->second.at(index)];
    }
    return boost::none;
}
boost::optional<QColor> FfzBadges::getBadgeColor(const UserId &id,
                                                 const int index)
{
    std::shared_lock lock(this->mutex_);

    auto badgeIt = this->badgeMap.find(id.string);
    if (badgeIt != this->badgeMap.end())
    {
        auto colorIt = this->colorMap.find(badgeIt->second.at(index));
        if (colorIt != this->colorMap.end())
        {
            return colorIt->second;
        }
        return boost::none;
    }
    return boost::none;
}
int FfzBadges::getNumBadges(const UserId &id)
{
    std::shared_lock lock(this->mutex_);

    auto it = this->badgeMap.find(id.string);
    if (it != this->badgeMap.end())
    {
        return it->second.size();
    }
    return 0;
}
void FfzBadges::loadFfzBadges()
{
    static QUrl url("https://api.frankerfacez.com/v1/badges/ids");

    NetworkRequest(url)
        .onSuccess([this](auto result) -> Outcome {
            std::unique_lock lock(this->mutex_);

            auto jsonRoot = result.parseJson();
            int index = 0;
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

                this->badges.push_back(
                    std::make_shared<const Emote>(std::move(emote)));
                this->colorMap[index] =
                    QColor(jsonBadge.value("color").toString());

                auto badgeId = QString::number(jsonBadge.value("id").toInt());
                for (const auto &user : jsonRoot.value("users")
                                            .toObject()
                                            .value(badgeId)
                                            .toArray())
                {
                    if (this->badgeMap.find(QString::number(user.toInt())) !=
                        this->badgeMap.end())
                    {
                        this->badgeMap[QString::number(user.toInt())].push_back(
                            jsonBadge.value("id").toInt() - 1);
                    }
                    else
                    {
                        this->badgeMap[QString::number(user.toInt())] =
                            std::vector<int>();
                        this->badgeMap[QString::number(user.toInt())].push_back(
                            jsonBadge.value("id").toInt() - 1);
                    }
                }
                ++index;
            }

            return Success;
        })
        .execute();
}

}  // namespace chatterino
