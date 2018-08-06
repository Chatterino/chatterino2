#include "TwitchBadges.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QThread>
#include "common/NetworkRequest.hpp"

namespace chatterino {

TwitchBadges::TwitchBadges()
{
}

void TwitchBadges::initialize(Settings &settings, Paths &paths)
{
    this->loadTwitchBadges();
}

void TwitchBadges::loadTwitchBadges()
{
    static QString url("https://badges.twitch.tv/v1/badges/global/display?language=en");

    NetworkRequest req(url);
    req.setCaller(QThread::currentThread());
    req.onSuccess([this](auto result) -> Outcome {
        auto root = result.parseJson();
        QJsonObject sets = root.value("badge_sets").toObject();

        for (QJsonObject::iterator it = sets.begin(); it != sets.end(); ++it) {
            QJsonObject versions = it.value().toObject().value("versions").toObject();

            for (auto versionIt = std::begin(versions); versionIt != std::end(versions);
                 ++versionIt) {
                auto emote =
                    Emote{{""},
                          ImageSet{
                              Image::fromUrl({root.value("image_url_1x").toString()}, 1),
                              Image::fromUrl({root.value("image_url_2x").toString()}, 0.5),
                              Image::fromUrl({root.value("image_url_4x").toString()}, 0.25),
                          },
                          Tooltip{root.value("description").toString()},
                          Url{root.value("clickURL").toString()}};
                // "title"
                // "clickAction"

                QJsonObject versionObj = versionIt.value().toObject();
                this->badges.emplace(versionIt.key(), std::make_shared<Emote>(emote));
            }
        }

        return Success;
    });

    req.execute();
}

}  // namespace chatterino
