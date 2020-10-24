#include "FfzBadges.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QThread>
#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "messages/Emote.hpp"

#include <QUrl>

#include <map>

namespace chatterino {
void FfzBadges::initialize(Settings &settings, Paths &paths)
{
    this->loadFfzBadges();
}

FfzBadges::FfzBadges()
{
}

boost::optional<EmotePtr> FfzBadges::getBadge(const UserId &id)
{
    auto it = this->badgeMap.find(id.string);
    if (it != this->badgeMap.end())
    {
        return this->badges[it->second];
    }
    return boost::none;
}
boost::optional<QColor> FfzBadges::getBadgeColor(const UserId &id)
{
    auto badgeIt = this->badgeMap.find(id.string);
    if (badgeIt != this->badgeMap.end())
    {
        auto colorIt = this->colorMap.find(badgeIt->second);
        if (colorIt != this->colorMap.end())
        {
            return colorIt->second;
        }
        return boost::none;
    }
    return boost::none;
}

void FfzBadges::loadFfzBadges()
{
    static QUrl url("https://api.frankerfacez.com/v1/badges/ids");

    NetworkRequest(url)
        .onSuccess([this](auto result) -> Outcome {
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
                    this->badgeMap[QString::number(user.toInt())] = index;
                }
                ++index;
            }

            return Success;
        })
        .execute();
}

}  // namespace chatterino
