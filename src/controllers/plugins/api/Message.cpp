#include "controllers/plugins/api/Message.hpp"

#include "Application.hpp"
#include "messages/MessageElement.hpp"

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "controllers/plugins/SolTypes.hpp"
#    include "messages/Message.hpp"

#    include <sol/sol.hpp>

namespace {

using namespace chatterino;

QDateTime datetimeFromOffset(qint64 offset)
{
    auto dt = QDateTime::fromMSecsSinceEpoch(offset);

#    ifdef CHATTERINO_WITH_TESTS
    if (getApp()->isTest())
    {
        return dt.toUTC();
    }
#    endif

    return dt;
}

MessageColor tryMakeMessageColor(const QString &name,
                                 MessageColor fallback = MessageColor::Text)
{
    if (name.isEmpty())
    {
        return fallback;
    }
    if (name == u"text")
    {
        return MessageColor::Text;
    }
    if (name == u"link")
    {
        return MessageColor::Link;
    }
    if (name == u"system")
    {
        return MessageColor::System;
    }
    // custom
    return QColor(name);
}

template <typename T>
T requiredGet(const sol::table &tbl, auto &&key)
{
    auto v = tbl.get<sol::optional<T>>(std::forward<decltype(key)>(key));
    if (!v)
    {
        throw std::runtime_error(std::string{"Missing required property: "} +
                                 key);
    }
    return *std::move(v);
}

std::unique_ptr<TextElement> textElementFromTable(const sol::table &tbl)
{
    return std::make_unique<TextElement>(
        requiredGet<QString>(tbl, "text"),
        tbl.get_or("flags", MessageElementFlag::Text),
        tryMakeMessageColor(tbl.get_or("color", QString{})),
        tbl.get_or("style", FontStyle::ChatMedium));
}

std::unique_ptr<SingleLineTextElement> singleLineTextElementFromTable(
    const sol::table &tbl)
{
    return std::make_unique<SingleLineTextElement>(
        requiredGet<QString>(tbl, "text"),
        tbl.get_or("flags", MessageElementFlag::Text),
        tryMakeMessageColor(tbl.get_or("color", QString{})),
        tbl.get_or("style", FontStyle::ChatMedium));
}

std::unique_ptr<MentionElement> mentionElementFromTable(const sol::table &tbl)
{
    // no flags!
    return std::make_unique<MentionElement>(
        requiredGet<QString>(tbl, "display_name"),
        requiredGet<QString>(tbl, "login_name"),
        tryMakeMessageColor(requiredGet<QString>(tbl, "fallback_color")),
        tryMakeMessageColor(requiredGet<QString>(tbl, "user_color")));
}

std::unique_ptr<TimestampElement> timestampElementFromTable(
    const sol::table &tbl)
{
    // no flags!
    auto time = tbl.get<std::optional<qint64>>("time");
    if (time)
    {
        return std::make_unique<TimestampElement>(
            datetimeFromOffset(*time).time());
    }
    return std::make_unique<TimestampElement>();
}

std::unique_ptr<TwitchModerationElement> twitchModerationElementFromTable()
{
    // no flags!
    return std::make_unique<TwitchModerationElement>();
}

std::unique_ptr<LinebreakElement> linebreakElementFromTable(
    const sol::table &tbl)
{
    return std::make_unique<LinebreakElement>(
        tbl.get_or("flags", MessageElementFlag::None));
}

std::unique_ptr<ReplyCurveElement> replyCurveElementFromTable()
{
    // no flags!
    return std::make_unique<ReplyCurveElement>();
}

std::unique_ptr<MessageElement> elementFromTable(const sol::table &tbl)
{
    auto type = requiredGet<QString>(tbl, "type");
    std::unique_ptr<MessageElement> el;
    if (type == u"text")
    {
        el = textElementFromTable(tbl);
    }
    else if (type == u"single-line-text")
    {
        el = singleLineTextElementFromTable(tbl);
    }
    else if (type == u"mention")
    {
        el = mentionElementFromTable(tbl);
    }
    else if (type == u"timestamp")
    {
        el = timestampElementFromTable(tbl);
    }
    else if (type == u"twitch-moderation")
    {
        el = twitchModerationElementFromTable();
    }
    else if (type == u"linebreak")
    {
        el = linebreakElementFromTable(tbl);
    }
    else if (type == u"reply-curve")
    {
        el = replyCurveElementFromTable();
    }
    else
    {
        throw std::runtime_error("Invalid message type");
    }
    assert(el);

    el->setTrailingSpace(tbl.get_or("trailing_space", true));
    el->setTooltip(tbl.get_or("tooltip", QString{}));

    return el;
}

std::shared_ptr<Message> messageFromTable(const sol::table &tbl)
{
    auto msg = std::make_shared<Message>();
    msg->flags = tbl.get_or("flags", MessageFlag::None);

    // This takes a UTC offset (not the milliseconds since the start of the day)
    auto parseTime = tbl.get<std::optional<qint64>>("parse_time");
    if (parseTime)
    {
        msg->parseTime = datetimeFromOffset(*parseTime).time();
    }

    msg->id = tbl.get_or("id", QString{});
    msg->searchText = tbl.get_or("search_text", QString{});
    msg->messageText = tbl.get_or("message_text", QString{});
    msg->loginName = tbl.get_or("login_name", QString{});
    msg->displayName = tbl.get_or("display_name", QString{});
    msg->localizedName = tbl.get_or("localized_name", QString{});
    msg->userID = tbl.get_or("user_id", QString{});
    // missing: timeoutUser
    msg->channelName = tbl.get_or("channel_name", QString{});

    auto usernameColor = tbl.get_or("username_color", QString{});
    if (!usernameColor.isEmpty())
    {
        msg->usernameColor = QColor(usernameColor);
    }

    auto serverReceivedTime =
        tbl.get<std::optional<qint64>>("server_received_time");
    if (serverReceivedTime)
    {
        msg->serverReceivedTime = datetimeFromOffset(*serverReceivedTime);
    }

    // missing: badges
    // missing: badgeInfos

    // we construct a color on the fly here
    auto highlightColor = tbl.get_or("highlight_color", QString{});
    if (!highlightColor.isEmpty())
    {
        msg->highlightColor = std::make_shared<QColor>(highlightColor);
    }

    // missing: replyThread
    // missing: replyParent
    // missing: count

    auto elements = tbl.get<std::optional<sol::table>>("elements");
    if (elements)
    {
        auto size = elements->size();
        for (size_t i = 1; i <= size; i++)
        {
            msg->elements.emplace_back(
                elementFromTable(elements->get<sol::table>(i)));
        }
    }

    // missing: reward
    return msg;
}

}  // namespace

namespace chatterino::lua::api::message {

void createUserType(sol::table &c2)
{
    c2.new_usertype<Message>("Message",
                             sol::factories([](const sol::table &tbl) {
                                 return messageFromTable(tbl);
                             }));
}

}  // namespace chatterino::lua::api::message

#endif
