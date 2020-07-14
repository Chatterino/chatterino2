#pragma once

#include "common/Aliases.hpp"
#include "messages/Image.hpp"
#include "messages/ImageSet.hpp"

#define TWITCH_CHANNEL_POINT_REWARD_URL(x)                                  \
    QString("https://static-cdn.jtvnw.net/custom-reward-images/default-%1") \
        .arg(x)

namespace chatterino {
class ChannelPointReward
{
public:
    ChannelPointReward(const rapidjson::Value &reward);
    ChannelPointReward() = delete;

    QString getRewardId() const;
    QString getChannelId() const;
    QString getRewardTitle() const;
    int getRewardCost() const;
    ImageSet getRewardImage() const;

private:
    QString rewardId;
    QString channelId;
    QString rewardTitle;
    int rewardCost;
    ImageSet rewardImage;

    QString parseImage(const rapidjson::Value &image, const char *key);
};

}  // namespace chatterino
