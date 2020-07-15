#pragma once

#include "common/Aliases.hpp"
#include "messages/Image.hpp"
#include "messages/ImageSet.hpp"

#define TWITCH_CHANNEL_POINT_REWARD_URL(x)                                  \
    QString("https://static-cdn.jtvnw.net/custom-reward-images/default-%1") \
        .arg(x)

namespace chatterino {
struct ChannelPointReward {
    ChannelPointReward(rapidjson::Value &reward);
    ChannelPointReward() = delete;
    QString id;
    QString channelId;
    QString title;
    int cost;
    ImageSet image;
};

}  // namespace chatterino
