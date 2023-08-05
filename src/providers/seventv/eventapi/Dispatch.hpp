#pragma once

#include "providers/seventv/eventapi/Subscription.hpp"
#include "providers/seventv/SeventvCosmetics.hpp"

#include <QJsonObject>
#include <QString>

namespace chatterino::seventv::eventapi {

// https://github.com/SevenTV/EventAPI/tree/ca4ff15cc42b89560fa661a76c5849047763d334#message-payload
struct Dispatch {
    SubscriptionType type;
    QJsonObject body;
    QString id;
    // it's okay for this to be empty
    QString actorName;

    Dispatch(QJsonObject obj);
};

struct EmoteAddDispatch {
    QString emoteSetID;
    QString actorName;
    QJsonObject emoteJson;
    QString emoteID;

    EmoteAddDispatch(const Dispatch &dispatch, QJsonObject emote);

    bool validate() const;
};

struct EmoteRemoveDispatch {
    QString emoteSetID;
    QString actorName;
    QString emoteName;
    QString emoteID;

    EmoteRemoveDispatch(const Dispatch &dispatch, QJsonObject emote);

    bool validate() const;
};

struct EmoteUpdateDispatch {
    QString emoteSetID;
    QString actorName;
    QString emoteID;
    QString oldEmoteName;
    QString emoteName;

    EmoteUpdateDispatch(const Dispatch &dispatch, QJsonObject oldValue,
                        QJsonObject value);

    bool validate() const;
};

struct UserConnectionUpdateDispatch {
    QString userID;
    QString actorName;
    QString oldEmoteSetID;
    QString emoteSetID;
    size_t connectionIndex;

    UserConnectionUpdateDispatch(const Dispatch &dispatch,
                                 const QJsonObject &update,
                                 size_t connectionIndex);

    bool validate() const;
};

struct CosmeticCreateDispatch {
    QJsonObject data;
    CosmeticKind kind;

    CosmeticCreateDispatch(const Dispatch &dispatch);

    bool validate() const;
};

struct EntitlementCreateDeleteDispatch {
    /** id of the user */
    QString userID;
    QString userName;
    /** id of the entitlement */
    QString refID;
    CosmeticKind kind;

    EntitlementCreateDeleteDispatch(const Dispatch &dispatch);

    bool validate() const;
};

}  // namespace chatterino::seventv::eventapi
