#include "ChannelPointReward.hpp"
#include "util/RapidjsonHelpers.hpp"

namespace chatterino {

ChannelPointReward::ChannelPointReward(const rapidjson::Value &reward)
{
    assert(rj::getSafe(reward, "id", this->rewardId));
    assert(rj::getSafe(reward, "channel_id", this->channelId));
    assert(rj::getSafe(reward, "title", this->rewardTitle));
    assert(rj::getSafe(reward, "cost", this->rewardCost));

    rapidjson::Value image;
    if (rj::getSafe(reward, "image", image))
    {
        this->rewardImage = ImageSet{
            Image::fromUrl({parseImage(image, "url_1x")}, 1),
            Image::fromUrl({parseImage(image, "url_2x")}, 0.5),
            Image::fromUrl({parseImage(image, "url_4x")}, 0.25),
        };
    }
    else
    {
        static const ImageSet defaultImage{
            Image::fromUrl({TWITCH_CHANNEL_POINT_REWARD_URL(1)}, 1),
            Image::fromUrl({TWITCH_CHANNEL_POINT_REWARD_URL(2)}, 0.5),
            Image::fromUrl({TWITCH_CHANNEL_POINT_REWARD_URL(4)}, 0.25)};
        this->rewardImage = defaultImage;
    }
}

QString ChannelPointReward::getRewardId() const
{
    return rewardId;
}

QString ChannelPointReward::getChannelId() const
{
    return channelId;
}

QString ChannelPointReward::getRewardTitle() const
{
    return rewardTitle;
}

int ChannelPointReward::getRewardCost() const
{
    return rewardCost;
}

ImageSet ChannelPointReward::getRewardImage() const
{
    return rewardImage;
}

QString ChannelPointReward::parseImage(const rapidjson::Value &image,
                                       const char *key)
{
    QString url;
    assert(rj::getSafe(image, key, url));

    return url;
}

}  // namespace chatterino
