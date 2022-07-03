#pragma once

#include <QJsonObject>
#include <QString>

#include <boost/optional.hpp>
#include <magic_enum.hpp>

#include "providers/seventv/eventapimessages/EventApiEmoteData.hpp"

namespace chatterino {
struct EventApiEmoteUpdate {
    enum class Action {
        Add,
        Remove,
        Update,

        INVALID,
    };

    QJsonObject json;

    QString channel;
    QString emoteId;
    QString actor;
    QString emoteName;

    Action action;
    QString actionString;

    boost::optional<EventApiEmoteData> emote;

    EventApiEmoteUpdate(QJsonObject _json);
};
}  // namespace chatterino

template <>
constexpr magic_enum::customize::customize_t
    magic_enum::customize::enum_name<chatterino::EventApiEmoteUpdate::Action>(
        chatterino::EventApiEmoteUpdate::Action value) noexcept
{
    switch (value)
    {
        case chatterino::EventApiEmoteUpdate::Action::Add:
            return "ADD";

        case chatterino::EventApiEmoteUpdate::Action::Remove:
            return "REMOVE";

        case chatterino::EventApiEmoteUpdate::Action::Update:
            return "UPDATE";

        default:
            return default_tag;
    }
}
