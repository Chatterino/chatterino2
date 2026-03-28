#include "controllers/filters/lang/expressions/ValueAccessorExpression.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "controllers/filters/lang/Types.hpp"
#include "messages/Message.hpp"
#include "messages/MessageFlag.hpp"
#include "providers/twitch/TwitchBadge.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"

#include <QString>

namespace {

using namespace chatterino;
using namespace filters;
using namespace Qt::Literals;

// Type to create the QVariant as (e.g. uint64_t narrows to int).
template <typename T>
struct Narrow {
    using Type = T;
};
template <typename T>
    requires(!std::is_void_v<typename filters::detail::TypeTraits<T>::Narrow>)
struct Narrow<T> {
    using Type = filters::detail::TypeTraits<T>::Narrow;
};

template <typename T>
QVariant makeVariantFor(T &&v)
{
    return QVariant::fromValue(std::forward<typename Narrow<T>::Type &&>(v));
}

struct AccessorExpressionBase : public Expression {
    PossibleType synthesizeType() const override
    {
        if (this->type_)
        {
            return TypeClass{*this->type_};
        }
        return IllTyped{.expr = this,
                        .message = u"Invalid access: " % this->name};
    }

    QString debug() const override
    {
        return u"Accessor(" % this->name % ')';
    }

    QString filterString() const override
    {
        return this->name;
    }

protected:
    AccessorExpressionBase(QString name, std::optional<Type> type)
        : name(std::move(name))
        , type_(type)
    {
    }

    QString name;
    std::optional<Type> type_;  // NOLINT(readability-identifier-naming)
};

template <auto Fn>
struct AccessorExpression final : public AccessorExpressionBase {
    AccessorExpression(QString name, std::optional<Type> type)
        : AccessorExpressionBase(std::move(name), type)
    {
    }

    QVariant execute(const RunContext &ctx) override
    {
        return Fn(ctx);
    }
};

using ExpressionCreator = std::unique_ptr<AccessorExpressionBase>(QString name);

template <auto Fn>
ExpressionCreator *functionAccessor()
{
    return +[](QString name) -> std::unique_ptr<AccessorExpressionBase> {
        return std::make_unique<AccessorExpression<Fn>>(
            std::move(name),
            TYPE_OF_V<std::invoke_result_t<decltype(Fn), const RunContext &>>);
    };
}

template <auto Ptr>
ExpressionCreator *memberAccessor()
{
    return functionAccessor<[](const RunContext &ctx) {
        return ctx.message.*Ptr;
    }>();
}

template <MessageFlag Flag>
ExpressionCreator *flagAccessor()
{
    return functionAccessor<[](const RunContext &ctx) {
        return ctx.message.flags.has(Flag);
    }>();
}

using AccessorMap = std::map<QString, ExpressionCreator *>;
const AccessorMap &accessorMap()
{
    static AccessorMap map{
        // author.*
        {u"author.badges"_s, functionAccessor<[](const RunContext &ctx) {
             QStringList badges(
                 static_cast<qsizetype>(ctx.message.twitchBadges.size()));
             for (const auto &e : ctx.message.twitchBadges)
             {
                 badges.emplace_back(e.key_);
             }
             return badges;
         }>()},
        {u"author.external_badges"_s,
         memberAccessor<&Message::externalBadges>()},
        {u"author.color"_s, memberAccessor<&Message::usernameColor>()},
        {u"author.name"_s, memberAccessor<&Message::displayName>()},
        {u"author.user_id"_s, memberAccessor<&Message::userID>()},
        {u"author.no_color"_s, functionAccessor<[](const RunContext &ctx) {
             return !ctx.message.usernameColor.isValid();
         }>()},
        {u"author.subbed"_s, functionAccessor<[](const RunContext &ctx) {
             return std::ranges::any_of(
                 ctx.message.twitchBadges, [](const auto &it) {
                     return it.key_ == u"subscriber" || it.key_ == u"founder";
                 });
         }>()},
        {u"author.sub_length"_s, functionAccessor<[](const RunContext &ctx) {
             auto it = ctx.message.twitchBadgeInfos.find(u"subscriber"_s);
             if (it == ctx.message.twitchBadgeInfos.end())
             {
                 it = ctx.message.twitchBadgeInfos.find(u"founder"_s);
             }
             if (it == ctx.message.twitchBadgeInfos.end())
             {
                 return 0;
             }
             return it->second.toInt();
         }>()},

        // channel.*
        {u"channel.live"_s, functionAccessor<[](const RunContext &ctx) {
             auto *tc = dynamic_cast<TwitchChannel *>(ctx.channel);
             if (tc)
             {
                 return tc->isLive();
             }
             return false;
         }>()},
        {u"channel.name"_s, memberAccessor<&Message::channelName>()},
        {u"channel.watching"_s, functionAccessor<[](const RunContext &ctx) {
             auto chan = getApp()->getTwitch()->getWatchingChannel().get();
             return !chan->getName().isEmpty() &&
                    chan->getName().compare(ctx.message.channelName,
                                            Qt::CaseInsensitive) == 0;
         }>()},

        // flags.*
        {u"flags.action"_s, flagAccessor<MessageFlag::Action>()},
        {u"flags.highlighted"_s, flagAccessor<MessageFlag::Highlighted>()},
        {u"flags.points_redeemed"_s,
         flagAccessor<MessageFlag::RedeemedHighlight>()},
        {u"flags.sub_message"_s, flagAccessor<MessageFlag::Subscription>()},
        {u"flags.system_message"_s, flagAccessor<MessageFlag::System>()},
        {u"flags.reward_message"_s,
         flagAccessor<MessageFlag::RedeemedChannelPointReward>()},
        {u"flags.first_message"_s, flagAccessor<MessageFlag::FirstMessage>()},
        {u"flags.elevated_message"_s,
         flagAccessor<MessageFlag::ElevatedMessage>()},
        {u"flags.hype_chat"_s, flagAccessor<MessageFlag::ElevatedMessage>()},
        {u"flags.cheer_message"_s, flagAccessor<MessageFlag::CheerMessage>()},
        {u"flags.whisper"_s, flagAccessor<MessageFlag::Whisper>()},
        {u"flags.reply"_s, flagAccessor<MessageFlag::ReplyMessage>()},
        {u"flags.automod"_s, flagAccessor<MessageFlag::AutoMod>()},
        {u"flags.restricted"_s, flagAccessor<MessageFlag::RestrictedMessage>()},
        {u"flags.monitored"_s, flagAccessor<MessageFlag::MonitoredMessage>()},
        {u"flags.shared"_s, flagAccessor<MessageFlag::SharedMessage>()},
        {u"flags.similar"_s, flagAccessor<MessageFlag::Similar>()},
        {u"flags.watch_streak"_s, flagAccessor<MessageFlag::WatchStreak>()},

        // message.*
        {u"message.content"_s, memberAccessor<&Message::messageText>()},
        {u"message.length"_s, functionAccessor<[](const RunContext &ctx) {
             return ctx.message.messageText.length();
         }>()},

        // reward.*
        {u"reward.cost"_s, functionAccessor<[](const RunContext &ctx) {
             auto r = ctx.message.reward;
             if (r)
             {
                 return r->cost;
             }
             return -1;
         }>()},
        {u"reward.id"_s, functionAccessor<[](const RunContext &ctx) {
             auto r = ctx.message.reward;
             if (r)
             {
                 return r->id;
             }
             return QString{};
         }>()},
        {u"reward.title"_s, functionAccessor<[](const RunContext &ctx) {
             auto r = ctx.message.reward;
             if (r)
             {
                 return r->title;
             }
             return QString{};
         }>()},
    };
    return map;
}

}  // namespace

namespace chatterino::filters {

std::unique_ptr<Expression> createValueAccessorExpression(const QString &name)
{
    const auto &map = accessorMap();
    auto it = map.find(name);
    if (it == map.end())
    {
        // FIXME: return an error here
        return std::unique_ptr<Expression>{new AccessorExpression<[](auto &&) {
            return false;
        }>(name, std::nullopt)};
    }
    return it->second(it->first);
}

}  // namespace chatterino::filters
