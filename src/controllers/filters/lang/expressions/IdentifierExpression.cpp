#include "controllers/filters/lang/expressions/IdentifierExpression.hpp"

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

template <typename T>
struct Narrow {
    using Type = T;
};
template <>
struct Narrow<unsigned int> : Narrow<int> {
};
template <>
struct Narrow<long> : Narrow<int> {
};
template <>
struct Narrow<unsigned long> : Narrow<int> {
};
template <>
struct Narrow<long long> : Narrow<int> {
};
template <>
struct Narrow<unsigned long long> : Narrow<int> {
};

template <typename T>
QVariant makeVariantFor(T &&v)
{
    return QVariant::fromValue(std::forward<typename Narrow<T>::Type>(v));
}

struct Accessor {
    /// Create an accessor from a function. The function should not return a
    /// QVariant but the type that should be contained in it (e.g. `QString`).
    Accessor(std::invocable<RunContext> auto &&fn)
        : fn([fn = std::forward<decltype(fn)>(fn)](RunContext ctx) {
            return makeVariantFor(fn(ctx));
        })
    {
    }

    /// Create an invalid accessor
    Accessor()
        : fn([](RunContext /* ctx */) {
            return false;
        })
    {
    }

    std::function<QVariant(RunContext)> fn;
};

struct IdentifierExpression final : public Expression {
    IdentifierExpression(QString name, std::optional<Type> type,
                         Accessor accessor)
        : name(std::move(name))
        , type(type)
        , accessor(std::move(accessor))
    {
    }

    PossibleType synthesizeType() const override
    {
        if (this->type)
        {
            return TypeClass{*this->type};
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

    QVariant execute(RunContext context) const override
    {
        return this->accessor.fn(context);
    }

private:
    QString name;
    std::optional<Type> type;
    Accessor accessor;
};

template <auto Ptr>
auto memberAccessor(RunContext ctx)
{
    return ctx.message.*Ptr;
}

template <MessageFlag Flag>
bool flagAccessor(RunContext ctx)
{
    return ctx.message.flags.has(Flag);
}

using AccessorMap = std::map<QString, std::pair<Type, Accessor>>;
const AccessorMap &accessorMap()
{
    static AccessorMap map{
        // author.*
        {
            u"author.badges"_s,
            {
                Type::StringList,
                [](RunContext ctx) {
                    QStringList badges(static_cast<qsizetype>(
                        ctx.message.twitchBadges.size()));
                    for (const auto &e : ctx.message.twitchBadges)
                    {
                        badges.emplace_back(e.key_);
                    }
                    return badges;
                },
            },
        },
        {
            u"author.external_badges"_s,
            {Type::StringList, memberAccessor<&Message::externalBadges>},
        },
        {
            u"author.color"_s,
            {Type::Color, memberAccessor<&Message::usernameColor>},
        },
        {
            u"author.name"_s,
            {Type::String, memberAccessor<&Message::displayName>},
        },
        {
            u"author.user_id"_s,
            {Type::String, memberAccessor<&Message::userID>},
        },
        {
            u"author.no_color"_s,
            {
                Type::Bool,
                [](RunContext ctx) {
                    return !ctx.message.usernameColor.isValid();
                },
            },
        },
        {
            u"author.subbed"_s,
            {
                Type::Bool,
                [](RunContext ctx) {
                    return std::ranges::any_of(
                        ctx.message.twitchBadges, [](const auto &it) {
                            return it.key_ == u"subscriber" ||
                                   it.key_ == u"founder";
                        });
                },
            },
        },
        {
            u"author.sub_length"_s,
            {
                Type::Int,
                [](RunContext ctx) {
                    auto it =
                        ctx.message.twitchBadgeInfos.find(u"subscriber"_s);
                    if (it == ctx.message.twitchBadgeInfos.end())
                    {
                        it = ctx.message.twitchBadgeInfos.find(u"founder"_s);
                    }
                    if (it == ctx.message.twitchBadgeInfos.end())
                    {
                        return 0;
                    }
                    return it->second.toInt();
                },
            },
        },

        // bits.*
        {
            u"bits.amount"_s,
            {Type::Int, memberAccessor<&Message::bits>},
        },

        // channel.*
        {
            u"channel.live"_s,
            {
                Type::Bool,
                [](RunContext ctx) {
                    auto *tc = dynamic_cast<TwitchChannel *>(ctx.channel);
                    if (tc)
                    {
                        return tc->isLive();
                    }
                    return false;
                },
            },
        },
        {
            u"channel.name"_s,
            {Type::String, memberAccessor<&Message::channelName>},
        },
        {
            u"channel.watching"_s,
            {
                Type::Bool,
                [](RunContext ctx) {
                    auto chan =
                        getApp()->getTwitch()->getWatchingChannel().get();
                    return !chan->getName().isEmpty() &&
                           chan->getName().compare(ctx.message.channelName,
                                                   Qt::CaseInsensitive) == 0;
                },
            },
        },

        // flags.*
        {
            u"flags.action"_s,
            {Type::Bool, flagAccessor<MessageFlag::Action>},
        },
        {
            u"flags.highlighted"_s,
            {Type::Bool, flagAccessor<MessageFlag::Highlighted>},
        },
        {
            u"flags.points_redeemed"_s,
            {Type::Bool, flagAccessor<MessageFlag::RedeemedHighlight>},
        },
        {
            u"flags.sub_message"_s,
            {Type::Bool, flagAccessor<MessageFlag::Subscription>},
        },
        {
            u"flags.system_message"_s,
            {Type::Bool, flagAccessor<MessageFlag::System>},
        },
        {
            u"flags.reward_message"_s,
            {Type::Bool, flagAccessor<MessageFlag::RedeemedChannelPointReward>},
        },
        {
            u"flags.first_message"_s,
            {Type::Bool, flagAccessor<MessageFlag::FirstMessage>},
        },
        {
            u"flags.elevated_message"_s,
            {Type::Bool, flagAccessor<MessageFlag::ElevatedMessage>},
        },
        {
            u"flags.hype_chat"_s,
            {Type::Bool, flagAccessor<MessageFlag::ElevatedMessage>},
        },
        {
            u"flags.cheer_message"_s,
            {Type::Bool, flagAccessor<MessageFlag::CheerMessage>},
        },
        {
            u"flags.whisper"_s,
            {Type::Bool, flagAccessor<MessageFlag::Whisper>},
        },
        {
            u"flags.reply"_s,
            {Type::Bool, flagAccessor<MessageFlag::ReplyMessage>},
        },
        {
            u"flags.automod"_s,
            {Type::Bool, flagAccessor<MessageFlag::AutoMod>},
        },
        {
            u"flags.restricted"_s,
            {Type::Bool, flagAccessor<MessageFlag::RestrictedMessage>},
        },
        {
            u"flags.monitored"_s,
            {Type::Bool, flagAccessor<MessageFlag::MonitoredMessage>},
        },
        {
            u"flags.shared"_s,
            {Type::Bool, flagAccessor<MessageFlag::SharedMessage>},
        },
        {
            u"flags.similar"_s,
            {Type::Bool, flagAccessor<MessageFlag::Similar>},
        },
        {
            u"flags.watch_streak"_s,
            {Type::Bool, flagAccessor<MessageFlag::WatchStreak>},
        },
        {
            u"flags.announcement"_s,
            {Type::Bool, flagAccessor<MessageFlag::Announcement>},
        },

        // message.*
        {
            u"message.content"_s,
            {Type::String, memberAccessor<&Message::messageText>},
        },
        {
            u"message.length"_s,
            {
                Type::Int,
                [](RunContext ctx) {
                    return ctx.message.messageText.length();
                },
            },
        },

        // reward.*
        {
            u"reward.cost"_s,
            {
                Type::Int,
                [](RunContext ctx) {
                    auto r = ctx.message.reward;
                    if (r)
                    {
                        return r->cost;
                    }
                    return -1;
                },
            },
        },
        {
            u"reward.id"_s,
            {
                Type::String,
                [](RunContext ctx) {
                    auto r = ctx.message.reward;
                    if (r)
                    {
                        return r->id;
                    }
                    return QString{};
                },
            },
        },
        {
            u"reward.title"_s,
            {
                Type::String,
                [](RunContext ctx) {
                    auto r = ctx.message.reward;
                    if (r)
                    {
                        return r->title;
                    }
                    return QString{};
                },
            },
        },
    };
    return map;
}

}  // namespace

namespace chatterino::filters {

std::unique_ptr<Expression> createIdentifierExpression(const QString &name)
{
    const auto &map = accessorMap();
    auto it = map.find(name);
    if (it == map.end())
    {
        // FIXME: Return an error here immediately instead of failing when type-checking.
        return std::make_unique<IdentifierExpression>(name, std::nullopt,
                                                      Accessor());
    }
    return std::make_unique<IdentifierExpression>(it->first, it->second.first,
                                                  it->second.second);
}

}  // namespace chatterino::filters
