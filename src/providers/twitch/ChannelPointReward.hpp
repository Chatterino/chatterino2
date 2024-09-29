#pragma once

#include "messages/ImageSet.hpp"

#include <QJsonObject>

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
    bool isBits = false;
    QString emoteId;    // currently only for celebrations
    QString emoteName;  // currently only for celebrations

    struct {
        QString id;
        QString login;
        QString displayName;
    } user;

    QJsonObject toJson() const;
};

}  // namespace chatterino
