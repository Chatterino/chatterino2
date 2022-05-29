#pragma once

#include "common/Aliases.hpp"
#include "messages/Image.hpp"
#include "messages/ImageSet.hpp"

#include <QJsonObject>

#define TWITCH_CHANNEL_POINT_REWARD_URL(x)                                  \
    QString("https://static-cdn.jtvnw.net/custom-reward-images/default-%1") \
        .arg(x)

namespace chatterino {
struct ChannelPointReward {
    ChannelPointReward(const QJsonObject &redemption);
    ChannelPointReward() = delete;
    QString id;
    QString channelId;
    QString title;
    int cost;
    ImageSet image;
    bool isUserInputRequired = false;

    struct {
        QString id;
        QString login;
        QString displayName;
    } user;
};

}  // namespace chatterino
