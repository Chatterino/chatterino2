#include "TwitchBadges.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QThread>

#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "messages/Emote.hpp"

namespace chatterino {

TwitchBadges::TwitchBadges()
{
    this->loadTwitchBadges();
}

void TwitchBadges::loadTwitchBadges()
{
    std::lock_guard<std::mutex> lock(
        this->loadingMutex_);  //todo: verify no deadlock

    if (this->loading_)
        return;

    this->loading_ = true;

    static QString url(
        "https://badges.twitch.tv/v1/badges/global/display?language=en");

    NetworkRequest(url)
        .onSuccess([this](auto result) -> Outcome {
            {
                auto root = result.parseJson();
                auto badgeSets = this->badgeSets_.access();

                auto jsonSets = root.value("badge_sets").toObject();
                for (auto sIt = jsonSets.begin(); sIt != jsonSets.end(); ++sIt)
                {
                    auto key = sIt.key();
                    auto versions =
                        sIt.value().toObject().value("versions").toObject();

                    for (auto vIt = versions.begin(); vIt != versions.end();
                         ++vIt)
                    {
                        auto versionObj = vIt.value().toObject();

                        auto emote = Emote{
                            {""},
                            ImageSet{
                                Image::fromUrl({versionObj.value("image_url_1x")
                                                    .toString()},
                                               1),
                                Image::fromUrl({versionObj.value("image_url_2x")
                                                    .toString()},
                                               .5),
                                Image::fromUrl({versionObj.value("image_url_4x")
                                                    .toString()},
                                               .25),
                            },
                            Tooltip{versionObj.value("title").toString()},
                            Url{versionObj.value("click_url").toString()}};
                        // "title"
                        // "clickAction"

                        (*badgeSets)[key][vIt.key()] =
                            std::make_shared<Emote>(emote);
                    }
                }
            }
            this->loaded();
            return Success;
        })
        .execute();
}

void TwitchBadges::loaded()
{
    {
        std::lock_guard<std::mutex> lock(this->loadingMutex_);
        this->loading_ = false;
    }
    std::lock_guard<std::mutex> lock(this->queueMutex_);
    while (!this->callbackQueue_.empty())
    {
        auto callback = this->callbackQueue_.front();
        this->callbackQueue_.pop();
        this->getBadgeIcon(callback.first, callback.second);
    }
}

boost::optional<EmotePtr> TwitchBadges::badge(const QString &set,
                                              const QString &version) const
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

boost::optional<EmotePtr> TwitchBadges::badge(const QString &set) const
{
    auto badgeSets = this->badgeSets_.access();
    auto it = badgeSets->find(set);
    if (it != badgeSets->end())
    {
        if (it->second.size() > 0)
        {
            return it->second.begin()->second;
        }
    }
    return boost::none;
}

void TwitchBadges::getBadgeIcon(const QString &name, BadgeIconCallback callback)
{
    {
        std::lock_guard<std::mutex> lock(this->loadingMutex_);
        if (this->loading_)
        {
            std::lock_guard<std::mutex> lock(this->queueMutex_);
            this->callbackQueue_.push({name, std::move(callback)});
            return;
        }
    }

    if (this->badgesMap_.contains(name))
    {
        callback(name, this->badgesMap_[name]);
    }
    else
    {
        if (const auto badge = this->badge(name))
        {
            this->loadEmoteImage(name, (*badge)->images.getImage3(),
                                 std::move(callback));
        }
    }
}

void TwitchBadges::getBadgeIcon(const DisplayBadge &badge,
                                BadgeIconCallback callback)
{
    this->getBadgeIcon(badge.badgeName(), std::move(callback));
}

void TwitchBadges::getBadgeIcons(const QList<DisplayBadge> &badges,
                                 BadgeIconCallback callback)
{
    for (const auto &item : badges)
    {
        this->getBadgeIcon(item, callback);
    }
}

void TwitchBadges::loadEmoteImage(const QString &name, ImagePtr image,
                                  BadgeIconCallback &&callback)
{
    NetworkRequest(image->url().string)
        .concurrent()
        .cache()
        .onSuccess([this, name, callback](auto result) -> Outcome {
            std::lock_guard<std::mutex> lock(this->mapMutex_);
            auto data = result.getData();

            // const cast since we are only reading from it
            QBuffer buffer(const_cast<QByteArray *>(&data));
            buffer.open(QIODevice::ReadOnly);
            QImageReader reader(&buffer);

            QImage image;
            if (reader.imageCount() == 0 || !reader.read(&image))
            {
                return Failure;
            }

            auto icon = std::make_shared<QIcon>(QPixmap::fromImage(image));

            this->badgesMap_[name] = icon;
            callback(name, icon);

            return Success;
        })
        .execute();
}

TwitchBadges *TwitchBadges::instance_;

TwitchBadges *TwitchBadges::instance()
{
    if (TwitchBadges::instance_ == nullptr)
    {
        TwitchBadges::instance_ = new TwitchBadges();
    }

    return TwitchBadges::instance_;
}

}  // namespace chatterino
