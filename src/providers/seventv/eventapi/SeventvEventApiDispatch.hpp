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

    SeventvEventApiDispatch(QJsonObject obj);
};

struct SeventvEventApiEmoteAddDispatch {
    QString emoteSetID;
    QString actorName;
    QJsonObject emoteJson;
    QString emoteID;

    SeventvEventApiEmoteAddDispatch(const SeventvEventApiDispatch &dispatch,
                                    QJsonObject emote);

    bool validate() const;
};

struct SeventvEventApiEmoteRemoveDispatch {
    QString emoteSetID;
    QString actorName;
    QString emoteName;
    QString emoteID;

    SeventvEventApiEmoteRemoveDispatch(const SeventvEventApiDispatch &dispatch,
                                       QJsonObject emote);

    bool validate() const;
};

struct SeventvEventApiEmoteUpdateDispatch {
    QString emoteSetID;
    QString actorName;
    QString emoteID;
    QString oldEmoteName;
    QString emoteName;

    SeventvEventApiEmoteUpdateDispatch(const SeventvEventApiDispatch &dispatch,
                                       QJsonObject changeField);

    bool validate() const;
};

struct SeventvEventApiUserConnectionUpdateDispatch {
    QString userID;
    QString actorName;
    QString oldEmoteSetID;
    QString emoteSetID;

    SeventvEventApiUserConnectionUpdateDispatch(
        const SeventvEventApiDispatch &dispatch, const QJsonObject &update);

    bool validate() const;
};

}  // namespace chatterino
