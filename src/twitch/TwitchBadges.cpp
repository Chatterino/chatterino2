#include "TwitchBadges.hpp"

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QThread>

#include "messages/Emote.hpp"
#include "net/NetworkRequest.hpp"
#include "util/Outcome.hpp"

namespace chatterino
{
    void TwitchBadges::load()
    {
        NetworkRequest req(
            "https://badges.twitch.tv/v1/badges/global/display?language=en");

        req.setCaller(QThread::currentThread());

        req.onSuccess([this](auto result) -> Outcome {
            auto root = result.parseJson();
            auto badgeSets = this->badgeSets_.access();

            auto jsonSets = root.value("badge_sets").toObject();
            for (auto sIt = jsonSets.begin(); sIt != jsonSets.end(); ++sIt)
            {
                auto key = sIt.key();
                auto versions =
                    sIt.value().toObject().value("versions").toObject();

                for (auto vIt = versions.begin(); vIt != versions.end(); ++vIt)
                {
                    auto versionObj = vIt.value().toObject();

                    auto emote = Emote{{""},
                        ImageSet{
                            Image::fromUrl(
                                {versionObj.value("image_url_1x").toString()},
                                1),
                            Image::fromUrl(
                                {versionObj.value("image_url_2x").toString()},
                                .5),
                            Image::fromUrl(
                                {versionObj.value("image_url_4x").toString()},
                                .25),
                        },
                        Tooltip{versionObj.value("description").toString()},
                        Url{versionObj.value("click_url").toString()}};
                    // "title"
                    // "clickAction"

                    (*badgeSets)[key][vIt.key()] =
                        std::make_shared<Emote>(emote);
                }
            }

            qDebug() << "Loaded twitch badges.";

            return Success;
        });

        req.execute();
    }

    boost::optional<EmotePtr> TwitchBadges::badge(
        const QString& set, const QString& version) const
    {
        auto badgeSets = this->badgeSets_.access();
        auto it = badgeSets->find(set);
        if (it != badgeSets->end())
        {
            auto it2 = it->second.find(version);
            if (it2 != it->second.end())
            {
                return it2->second;
            }
        }
        return boost::none;
    }

    TwitchBadges& twitchBadges()
    {
        static TwitchBadges badges;
        return badges;
    }
}  // namespace chatterino
