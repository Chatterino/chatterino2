#include "GlobalBadges.hpp"
#include "common/NetworkRequest.hpp"
#include "messages/Emote.hpp"
#include "singletons/Resources.hpp"
#include "util/PostToThread.hpp"

namespace chatterino {
GlobalBadges::GlobalBadges()
    : badges_(new TwitchBadges)
{
    this->badges_->loaded.connect([this] {
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
    });
    this->update();
}

void GlobalBadges::update()
{
    std::lock_guard<std::mutex> lock(this->loadingMutex_);

    if (this->loading_)
        return;

    this->loading_ = true;
    this->badges_->loadTwitchBadges();
}

void GlobalBadges::loadEmoteImage(const QString &identifier, ImagePtr image,
                                  BadgeIconCallback &&callback)
{
    NetworkRequest(image->url().string)
        .concurrent()
        .cache()
        .onSuccess([this, identifier, callback](auto result) -> Outcome {
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

            this->badgesMap_[identifier] = icon;
            callback(identifier, icon);

            return Success;
        })
        .execute();
}

void GlobalBadges::getBadgeIcon(const QString &identifier,
                                BadgeIconCallback callback)
{
    {
        std::lock_guard<std::mutex> lock(this->loadingMutex_);
        if (this->loading_)
        {
            std::lock_guard<std::mutex> lock(this->queueMutex_);
            this->callbackQueue_.push({identifier, std::move(callback)});
            return;
        }
    }

    if (this->badgesMap_.contains(identifier))
    {
        callback(identifier, this->badgesMap_[identifier]);
    }
    else
    {
        auto parts = identifier.split(".");
        auto badge = this->badges_->badge(parts.at(0), parts.at(1));
        if (badge)
        {
            this->loadEmoteImage(identifier, (*badge)->images.getImage3(),
                                 std::move(callback));
        }
    }
}

void GlobalBadges::getBadgeIcon(const DisplayBadge &badge,
                                BadgeIconCallback callback)
{
    this->getBadgeIcon(badge.identifier(), std::move(callback));
}

void GlobalBadges::getBadgeIcons(const QList<DisplayBadge> &badges,
                                 BadgeIconCallback callback)
{
    QListIterator i(badges);
    while (i.hasNext())
    {
        this->getBadgeIcon(i.next(), callback);
    }
}

GlobalBadges *GlobalBadges::instance_;

GlobalBadges *GlobalBadges::instance()
{
    if (GlobalBadges::instance_ == nullptr)
    {
        GlobalBadges::instance_ = new GlobalBadges();
    }

    return GlobalBadges::instance_;
}

}  // namespace chatterino
