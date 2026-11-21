// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <magic_enum/magic_enum.hpp>
#include <QJsonObject>
#include <QString>

#include <cstdint>

namespace chatterino {

struct PubSubPinnedChatUpdatesV1Message {
    enum class Type : std::uint8_t {
        PinMessage,
        UnpinMessage,
        UpdateMessage,

        INVALID,
    };

    QString typeString;
    Type type = Type::INVALID;

    QJsonObject data;

    PubSubPinnedChatUpdatesV1Message(const QJsonObject &root);
};

}  // namespace chatterino

template <>
constexpr magic_enum::customize::customize_t
    magic_enum::customize::enum_name<  // NOLINT(readability-identifier-naming)
        chatterino::PubSubPinnedChatUpdatesV1Message::Type>(
        chatterino::PubSubPinnedChatUpdatesV1Message::Type value) noexcept
{
    switch (value)
    {
        case chatterino::PubSubPinnedChatUpdatesV1Message::Type::PinMessage:
            return "pin-message";
        case chatterino::PubSubPinnedChatUpdatesV1Message::Type::UnpinMessage:
            return "unpin-message";
        case chatterino::PubSubPinnedChatUpdatesV1Message::Type::UpdateMessage:
            return "update-message";
        default:
            return default_tag;  // NOLINT(clazy-rule-of-two-soft)
    }
}
