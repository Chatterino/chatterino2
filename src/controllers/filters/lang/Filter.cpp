#include "controllers/filters/lang/Filter.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "controllers/filters/lang/FilterParser.hpp"
#include "messages/Message.hpp"
#include "providers/twitch/TwitchBadge.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"

namespace chatterino::filters {

ContextMap buildContextMap(const MessagePtr &m, chatterino::Channel *channel)
{
    auto watchingChannel = chatterino::getApp()->twitch->watchingChannel.get();

    /* 
     * Looking to add a new identifier to filters? Here's what to do: 
     *  1. Update validIdentifiersMap in Tokenizer.hpp
     *  2. Add the identifier to the list below
     *  3. Add the type of the identifier to MESSAGE_TYPING_CONTEXT in Filter.hpp
     *  4. Add the value for the identifier to the ContextMap returned by this function
     * 
     * List of identifiers:
     *
     * author.badges
     * author.color
     * author.name
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
     *
     * message.content
     * message.length
     *
     * dankerino:
     * flags.webchat_detected
     *
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
        {"author.no_color", !m->usernameColor.isValid()},
        {"author.subbed", subscribed},
        {"author.sub_length", subLength},

        {"channel.name", m->channelName},
        {"channel.watching", watching},

        {"flags.highlighted", m->flags.has(MessageFlag::Highlighted)},
        {"flags.points_redeemed", m->flags.has(MessageFlag::RedeemedHighlight)},
        {"flags.sub_message", m->flags.has(MessageFlag::Subscription)},
        {"flags.system_message", m->flags.has(MessageFlag::System)},
        {"flags.reward_message",
         m->flags.has(MessageFlag::RedeemedChannelPointReward)},
        {"flags.first_message", m->flags.has(MessageFlag::FirstMessage)},
        {"flags.elevated_message", m->flags.has(MessageFlag::ElevatedMessage)},
        {"flags.cheer_message", m->flags.has(MessageFlag::CheerMessage)},
        {"flags.whisper", m->flags.has(MessageFlag::Whisper)},
        {"flags.reply", m->flags.has(MessageFlag::ReplyMessage)},
        {"flags.automod", m->flags.has(MessageFlag::AutoMod)},

        {"message.content", m->messageText},
        {"message.length", m->messageText.length()},

        {"flags.webchat_detected", m->flags.has(MessageFlag::WebchatDetected)},
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
