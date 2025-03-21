#include "controllers/filters/lang/Filter.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "controllers/filters/lang/FilterParser.hpp"
#include "messages/Message.hpp"
#include "providers/twitch/TwitchBadge.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"

namespace chatterino::filters {

const QMap<QString, Type> MESSAGE_TYPING_CONTEXT{
    {"author.badges", Type::StringList},
    {"author.color", Type::Color},
    {"author.name", Type::String},
    {"author.user_id", Type::String},
    {"author.no_color", Type::Bool},
    {"author.subbed", Type::Bool},
    {"author.sub_length", Type::Int},
    {"channel.name", Type::String},
    {"channel.watching", Type::Bool},
    {"channel.live", Type::Bool},
    {"flags.action", Type::Bool},
    {"flags.highlighted", Type::Bool},
    {"flags.points_redeemed", Type::Bool},
    {"flags.sub_message", Type::Bool},
    {"flags.system_message", Type::Bool},
    {"flags.reward_message", Type::Bool},
    {"flags.first_message", Type::Bool},
    {"flags.elevated_message", Type::Bool},
    {"flags.hype_chat", Type::Bool},
    {"flags.cheer_message", Type::Bool},
    {"flags.whisper", Type::Bool},
    {"flags.reply", Type::Bool},
    {"flags.automod", Type::Bool},
    {"flags.restricted", Type::Bool},
    {"flags.monitored", Type::Bool},
    {"flags.shared", Type::Bool},
    {"flags.similar", Type::Bool},
    {"message.content", Type::String},
    {"message.length", Type::Int},
    {"reward.title", Type::String},
    {"reward.cost", Type::Int},
    {"reward.id", Type::String},
};

ContextMap buildContextMap(const MessagePtr &m, chatterino::Channel *channel)
{
    auto watchingChannel = getApp()->getTwitch()->getWatchingChannel().get();

    /* 
     * Looking to add a new identifier to filters? Here's what to do: 
     *  1. Update validIdentifiersMap in Tokenizer.cpp
     *  2. Add the identifier to the list below
     *  3. Add the type of the identifier to MESSAGE_TYPING_CONTEXT in Filter.hpp
     *  4. Add the value for the identifier to the ContextMap returned by this function
     * 
     * List of identifiers:
     *
     * author.badges
     * author.color
     * author.name
     * author.user_id
     * author.no_color
     * author.subbed
     * author.sub_length
     *
     * channel.name
     * channel.watching
     *
     * flags.highlighted
     * flags.points_redeemed
     * flags.sub_message
     * flags.system_message
     * flags.reward_message
     * flags.first_message
     * flags.elevated_message
     * flags.cheer_message
     * flags.whisper
     * flags.reply
     * flags.automod
     * flags.restricted
     * flags.monitored
     * flags.shared
     *
     * message.content
     * message.length
     *
     * reward.title
     * reward.cost
     * reward.id
     */

    using MessageFlag = chatterino::MessageFlag;

    QStringList badges;
    badges.reserve(m->badges.size());
    for (const auto &e : m->badges)
    {
        badges << e.key_;
    }

    bool watching = !watchingChannel->getName().isEmpty() &&
                    watchingChannel->getName().compare(
                        m->channelName, Qt::CaseInsensitive) == 0;

    bool subscribed = false;
    int subLength = 0;
    for (const auto &subBadge : {"subscriber", "founder"})
    {
        if (!badges.contains(subBadge))
        {
            continue;
        }
        subscribed = true;
        if (m->badgeInfos.find(subBadge) != m->badgeInfos.end())
        {
            subLength = m->badgeInfos.at(subBadge).toInt();
        }
    }
    ContextMap vars = {
        {"author.badges", std::move(badges)},
        {"author.color", m->usernameColor},
        {"author.name", m->displayName},
        {"author.user_id", m->userID},
        {"author.no_color", !m->usernameColor.isValid()},
        {"author.subbed", subscribed},
        {"author.sub_length", subLength},

        {"channel.name", m->channelName},
        {"channel.watching", watching},

        {"flags.action", m->flags.has(MessageFlag::Action)},
        {"flags.highlighted", m->flags.has(MessageFlag::Highlighted)},
        {"flags.points_redeemed", m->flags.has(MessageFlag::RedeemedHighlight)},
        {"flags.sub_message", m->flags.has(MessageFlag::Subscription)},
        {"flags.system_message", m->flags.has(MessageFlag::System)},
        {"flags.reward_message",
         m->flags.has(MessageFlag::RedeemedChannelPointReward)},
        {"flags.first_message", m->flags.has(MessageFlag::FirstMessage)},
        {"flags.elevated_message", m->flags.has(MessageFlag::ElevatedMessage)},
        {"flags.hype_chat", m->flags.has(MessageFlag::ElevatedMessage)},
        {"flags.cheer_message", m->flags.has(MessageFlag::CheerMessage)},
        {"flags.whisper", m->flags.has(MessageFlag::Whisper)},
        {"flags.reply", m->flags.has(MessageFlag::ReplyMessage)},
        {"flags.automod", m->flags.has(MessageFlag::AutoMod)},
        {"flags.restricted", m->flags.has(MessageFlag::RestrictedMessage)},
        {"flags.monitored", m->flags.has(MessageFlag::MonitoredMessage)},
        {"flags.shared", m->flags.has(MessageFlag::SharedMessage)},
        {"flags.similar", m->flags.has(MessageFlag::Similar)},

        {"message.content", m->messageText},
        {"message.length", m->messageText.length()},
    };
    {
        auto *tc = dynamic_cast<TwitchChannel *>(channel);
        if (channel && !channel->isEmpty() && tc)
        {
            vars["channel.live"] = tc->isLive();
        }
        else
        {
            vars["channel.live"] = false;
        }
    }
    if (m->reward != nullptr)
    {
        vars["reward.title"] = m->reward->title;
        vars["reward.cost"] = m->reward->cost;
        vars["reward.id"] = m->reward->id;
    }
    else
    {
        vars["reward.title"] = "";
        vars["reward.cost"] = -1;
        vars["reward.id"] = "";
    }
    return vars;
}

FilterResult Filter::fromString(const QString &str)
{
    FilterParser parser(str);

    if (parser.valid())
    {
        auto exp = parser.release();
        auto typ = parser.returnType();
        return Filter(std::move(exp), typ);
    }

    return FilterError{parser.errors().join("\n")};
}

Filter::Filter(ExpressionPtr expression, Type returnType)
    : expression_(std::move(expression))
    , returnType_(returnType)
{
}

Type Filter::returnType() const
{
    return this->returnType_;
}

QVariant Filter::execute(const ContextMap &context) const
{
    return this->expression_->execute(context);
}

QString Filter::filterString() const
{
    return this->expression_->filterString();
}

QString Filter::debugString(const TypingContext &context) const
{
    return this->expression_->debug(context);
}

}  // namespace chatterino::filters
