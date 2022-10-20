#pragma once

#include <QJsonObject>
#include <QString>
#include "providers/seventv/SeventvEventApi.hpp"

namespace chatterino {
struct SeventvEventApiDispatch {
    SeventvEventApiSubscriptionType type =
        SeventvEventApiSubscriptionType::INVALID;
    QJsonObject body;
    QString id;
    QString actorName;

    SeventvEventApiDispatch(QJsonObject _data);
};

struct SeventvEventApiEmoteAddDispatch {
    QString emoteSetId;
    QString actorName;
    QJsonObject emoteJson;
    QString emoteId;

    SeventvEventApiEmoteAddDispatch(const SeventvEventApiDispatch &dispatch,
                                    QJsonObject emote);
};

struct SeventvEventApiEmoteRemoveDispatch {
    QString emoteSetId;
    QString actorName;
    QString emoteName;
    QString emoteId;

    SeventvEventApiEmoteRemoveDispatch(const SeventvEventApiDispatch &dispatch,
                                       QJsonObject emote);
};

struct SeventvEventApiEmoteUpdateDispatch {
    QString emoteSetId;
    QString actorName;
    QString emoteId;
    QString oldEmoteName;
    QString emoteName;

    SeventvEventApiEmoteUpdateDispatch(const SeventvEventApiDispatch &dispatch,
                                       QJsonObject emote);
};

struct SeventvEventApiUserConnectionUpdateDispatch {
    QString userId;
    QString actorName;
    QString oldEmoteSetId;
    QString emoteSetId;

    SeventvEventApiUserConnectionUpdateDispatch(
        const SeventvEventApiDispatch &dispatch, const QJsonObject &update);
};
}  // namespace chatterino
