// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "providers/twitch/pubsubmessages/PinnedChatUpdates.hpp"

#include "util/QMagicEnum.hpp"

namespace chatterino {

PubSubPinnedChatUpdatesV1Message::PubSubPinnedChatUpdatesV1Message(
    const QJsonObject &root)
    : typeString(root.value("type").toString())
    , data(root.value("data").toObject())
{
    auto oType = qmagicenum::enumCast<Type>(this->typeString);
    if (oType.has_value())
    {
        this->type = oType.value();
    }
}

}  // namespace chatterino
