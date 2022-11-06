#pragma once

#include "providers/seventv/eventapi/SeventvEventAPISubscription.hpp"

#include <QJsonObject>
#include <QString>

namespace chatterino {

// https://github.com/SevenTV/EventAPI/tree/ca4ff15cc42b89560fa661a76c5849047763d334#message-payload
struct SeventvEventAPIDispatch {
    const SeventvEventAPISubscriptionType type;
    const QJsonObject body;
    const QString id;
    // it's okay for this to be empty
    const QString actorName;

    SeventvEventAPIDispatch(QJsonObject obj);
};

struct SeventvEventAPIEmoteAddDispatch {
    const QString emoteSetID;
    const QString actorName;
    const QJsonObject emoteJson;
    const QString emoteID;

    SeventvEventAPIEmoteAddDispatch(const SeventvEventAPIDispatch &dispatch,
                                    QJsonObject emote);

    bool validate() const;
};

struct SeventvEventAPIEmoteRemoveDispatch {
    const QString emoteSetID;
    const QString actorName;
    const QString emoteName;
    const QString emoteID;

    SeventvEventAPIEmoteRemoveDispatch(const SeventvEventAPIDispatch &dispatch,
                                       QJsonObject emote);

    bool validate() const;
};

struct SeventvEventAPIEmoteUpdateDispatch {
    const QString emoteSetID;
    const QString actorName;
    const QString emoteID;
    const QString oldEmoteName;
    const QString emoteName;

    SeventvEventAPIEmoteUpdateDispatch(const SeventvEventAPIDispatch &dispatch,
                                       QJsonObject oldValue, QJsonObject value);

    bool validate() const;
};

struct SeventvEventAPIUserConnectionUpdateDispatch {
    const QString userID;
    const QString actorName;
    const QString oldEmoteSetID;
    const QString emoteSetID;
    const size_t connectionIndex;

    SeventvEventAPIUserConnectionUpdateDispatch(
        const SeventvEventAPIDispatch &dispatch, const QJsonObject &update,
        size_t connectionIndex);

    bool validate() const;
};

}  // namespace chatterino
