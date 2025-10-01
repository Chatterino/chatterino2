#include "providers/twitch/TwitchBadges.hpp"

#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "util/DisplayBadge.hpp"
#include "util/LoadPixmap.hpp"

#include <QBuffer>
#include <QFile>
#include <QIcon>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QThread>
#include <QUrlQuery>

namespace {

// From Twitch docs - expected size for a badge (1x)
constexpr QSize BADGE_BASE_SIZE(18, 18);

}  // namespace

namespace chatterino {

void TwitchBadges::loadTwitchBadges()
{
    assert(this->loaded_ == false);

    getHelix()->getGlobalBadges(
        [this](auto globalBadges) {
            auto badgeSets = this->badgeSets_.access();

            for (const auto &badgeSet : globalBadges.badgeSets)
            {
                const auto &setID = badgeSet.setID;
                for (const auto &version : badgeSet.versions)
                {
                    const auto &emote = Emote{
                        .name = EmoteName{},
                        .images =
                            ImageSet{
                                Image::fromUrl(version.imageURL1x, 1,
                                               BADGE_BASE_SIZE),
                                Image::fromUrl(version.imageURL2x, .5,
                                               BADGE_BASE_SIZE * 2),
                                Image::fromUrl(version.imageURL4x, .25,
                                               BADGE_BASE_SIZE * 4),
                            },
                        .tooltip = Tooltip{version.title},
                        .homePage = version.clickURL,
                    };
                    (*badgeSets)[setID][version.id] =
                        std::make_shared<Emote>(emote);
                }
            }

            this->loaded();
        },
        [this](auto error, auto message) {
            QString errorMessage("Failed to load global badges - ");

            switch (error)
            {
                case HelixGetGlobalBadgesError::Forwarded: {
                    errorMessage += message;
                }
                break;

                // This would most likely happen if the service is down, or if the JSON payload returned has changed format
                case HelixGetGlobalBadgesError::Unknown: {
                    errorMessage += "An unknown error has occurred.";
                }
                break;
            }
            qCWarning(chatterinoTwitch) << errorMessage;
            this->loadLocalBadges();
        });
}

void TwitchBadges::loadLocalBadges()
{
    QFile file(":/twitch-badges.json");
    if (!file.open(QFile::ReadOnly))
    {
        // Despite erroring out, we still want to reach the same point
        // Loaded should still be set to true to not build up an endless queue, and the quuee should still be flushed.
        qCWarning(chatterinoTwitch)
            << "Error loading Twitch Badges from the local backup file";
        this->loaded();
        return;
    }
    auto bytes = file.readAll();
    auto doc = QJsonDocument::fromJson(bytes);

    {
        const auto &root = doc.object();
        auto badgeSets = this->badgeSets_.access();

        for (auto setIt = root.begin(); setIt != root.end(); setIt++)
        {
            auto key = setIt.key();

            for (auto versionValue : setIt.value().toArray())
            {
                const auto versionObj = versionValue.toObject();
                auto id = versionObj["id"].toString();
                auto baseImage = versionObj["image"].toString();
                auto emote = Emote{
                    .name = {},
                    .images =
                        ImageSet{
                            Image::fromUrl({baseImage + '1'}, 1,
                                           BADGE_BASE_SIZE),
                            Image::fromUrl({baseImage + '2'}, .5,
                                           BADGE_BASE_SIZE * 2),
                            Image::fromUrl({baseImage + '3'}, .25,
                                           BADGE_BASE_SIZE * 4),
                        },
                    .tooltip = Tooltip{versionObj["title"].toString()},
                    .homePage = Url{versionObj["url"].toString()},
                };

                (*badgeSets)[key][id] = std::make_shared<Emote>(emote);
            }
        }
    }

    this->loaded();
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

std::optional<EmotePtr> TwitchBadges::badge(const QString &set,
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
    return std::nullopt;
}

std::optional<EmotePtr> TwitchBadges::badge(const QString &set) const
{
    auto badgeSets = this->badgeSets_.access();
    auto it = badgeSets->find(set);
    if (it != badgeSets->end())
    {
        const auto &badges = it->second;
        if (!badges.empty())
        {
            return badges.begin()->second;
        }
    }
    return std::nullopt;
}

void TwitchBadges::getBadgeIcon(const QString &name, BadgeIconCallback callback)
{
    {
        std::shared_lock loadedLock(this->loadedMutex_);

        if (!this->loaded_)
        {
            // Badges have not been loaded yet, store callback in a queue
            std::unique_lock queueLock(this->queueMutex_);
            this->callbackQueue_.emplace(name, std::move(callback));
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

void TwitchBadges::loadEmoteImage(const QString &name, const ImagePtr &image,
                                  BadgeIconCallback &&callback)
{
    loadPixmapFromUrl(image->url(),
                      [this, name, callback{std::move(callback)}](auto pixmap) {
                          auto icon = std::make_shared<QIcon>(pixmap);

                          {
                              std::unique_lock lock(this->badgesMutex_);
                              this->badgesMap_[name] = icon;
                          }

                          callback(name, icon);
                      });
}

}  // namespace chatterino
