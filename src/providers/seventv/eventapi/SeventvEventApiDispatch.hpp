#pragma once

#include "providers/seventv/eventapi/SeventvEventApiSubscription.hpp"

#include <QJsonObject>
#include <QString>

namespace chatterino {

// https://github.com/SevenTV/EventAPI/tree/ca4ff15cc42b89560fa661a76c5849047763d334#message-payload
struct SeventvEventApiDispatch {
    SeventvEventApiSubscriptionType type =
        SeventvEventApiSubscriptionType::INVALID;
    QJsonObject body;
    QString id;
    // it's okay for this to be empty
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

    bool validate() const;
};

struct SeventvEventApiEmoteRemoveDispatch {
    QString emoteSetId;
    QString actorName;
    QString emoteName;
    QString emoteId;

    SeventvEventApiEmoteRemoveDispatch(const SeventvEventApiDispatch &dispatch,
                                       QJsonObject emote);

    bool validate() const;
};

struct SeventvEventApiEmoteUpdateDispatch {
    QString emoteSetId;
    QString actorName;
    QString emoteId;
    QString oldEmoteName;
    QString emoteName;

    SeventvEventApiEmoteUpdateDispatch(const SeventvEventApiDispatch &dispatch,
                                       QJsonObject emote);

    bool validate() const;
};

struct SeventvEventApiUserConnectionUpdateDispatch {
    QString userId;
    QString actorName;
    QString oldEmoteSetId;
    QString emoteSetId;

    SeventvEventApiUserConnectionUpdateDispatch(
        const SeventvEventApiDispatch &dispatch, const QJsonObject &update);

    bool validate() const;
};

}  // namespace chatterino
