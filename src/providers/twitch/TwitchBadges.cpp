#include "TwitchBadges.hpp"

#include <QBuffer>
#include <QIcon>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QThread>
#include <QUrlQuery>

#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "common/QLogging.hpp"
#include "messages/Emote.hpp"

namespace chatterino {

TwitchBadges::TwitchBadges()
{
    this->loadTwitchBadges();
}

void TwitchBadges::loadTwitchBadges()
{
    assert(this->loaded_ == false);

    QUrl url("https://badges.twitch.tv/v1/badges/global/display");

    QUrlQuery urlQuery;
    urlQuery.addQueryItem("language", "en");
    url.setQuery(urlQuery);

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
        .onError([this](auto res) {
            qCDebug(chatterinoTwitch)
                << "Error loading Twitch Badges:" << res.status();
            // Despite erroring out, we still want to reach the same point
            // Loaded should still be set to true to not build up an endless queue, and the quuee should still be flushed.
            this->loaded();
        })
        .execute();
}

void TwitchBadges::loaded()
{
    std::unique_lock loadedLock(this->loadedMutex_);

    assert(this->loaded_ == false);

    this->loaded_ = true;

    // Flush callback queue
    std::unique_lock queueLock(this->queueMutex_);

    // Once we have gained unique access of the queue, we can release our unique access of the loaded mutex allowing future calls to read locked_
    loadedLock.unlock();

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
        std::shared_lock loadedLock(this->loadedMutex_);

        if (!this->loaded_)
        {
            // Badges have not been loaded yet, store callback in a queue
            std::unique_lock queueLock(this->queueMutex_);
            this->callbackQueue_.push({name, std::move(callback)});
            return;
        }
    }

    {
        std::shared_lock badgeLock(this->badgesMutex_);
        if (this->badgesMap_.contains(name))
        {
            callback(name, this->badgesMap_[name]);
            return;
        }
    }

    // Split string in format "name1/version1,name2/version2" to "name1", "version1"
    // If not in list+version form, name will remain the same
    auto targetBadge = name.split(",").at(0).split("/");

    const auto badge = targetBadge.size() == 2
                           ? this->badge(targetBadge.at(0), targetBadge.at(1))
                           : this->badge(targetBadge.at(0));

    if (badge)
    {
        this->loadEmoteImage(name, (*badge)->images.getImage3(),
                             std::move(callback));
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

            {
                std::unique_lock lock(this->badgesMutex_);
                this->badgesMap_[name] = icon;
            }

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
