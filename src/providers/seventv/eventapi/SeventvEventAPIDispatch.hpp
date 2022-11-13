#pragma once

#include "providers/seventv/eventapi/SeventvEventAPISubscription.hpp"

#include <QJsonObject>
#include <QString>

namespace chatterino {

// https://github.com/SevenTV/EventAPI/tree/ca4ff15cc42b89560fa661a76c5849047763d334#message-payload
struct SeventvEventAPIDispatch {
    SeventvEventAPISubscriptionType type;
    QJsonObject body;
    QString id;
    // it's okay for this to be empty
    QString actorName;

    SeventvEventAPIDispatch(QJsonObject obj);
};

struct SeventvEventAPIEmoteAddDispatch {
    QString emoteSetID;
    QString actorName;
    QJsonObject emoteJson;
    QString emoteID;

    SeventvEventAPIEmoteAddDispatch(const SeventvEventAPIDispatch &dispatch,
                                    QJsonObject emote);

    bool validate() const;
};

struct SeventvEventAPIEmoteRemoveDispatch {
    QString emoteSetID;
    QString actorName;
    QString emoteName;
    QString emoteID;

    SeventvEventAPIEmoteRemoveDispatch(const SeventvEventAPIDispatch &dispatch,
                                       QJsonObject emote);

    bool validate() const;
};

struct SeventvEventAPIEmoteUpdateDispatch {
    QString emoteSetID;
    QString actorName;
    QString emoteID;
    QString oldEmoteName;
    QString emoteName;

    SeventvEventAPIEmoteUpdateDispatch(const SeventvEventAPIDispatch &dispatch,
                                       QJsonObject oldValue, QJsonObject value);

    bool validate() const;
};

struct SeventvEventAPIUserConnectionUpdateDispatch {
    QString userID;
    QString actorName;
    QString oldEmoteSetID;
    QString emoteSetID;
    size_t connectionIndex;

    SeventvEventAPIUserConnectionUpdateDispatch(
        const SeventvEventAPIDispatch &dispatch, const QJsonObject &update,
        size_t connectionIndex);

    bool validate() const;
};

}  // namespace chatterino
