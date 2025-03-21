#include "controllers/commands/builtin/chatterino/Debugging.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "common/Env.hpp"
#include "common/Literals.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "messages/Image.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "messages/MessageElement.hpp"
#include "providers/twitch/PubSubActions.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Theme.hpp"
#include "singletons/Toasts.hpp"
#include "util/PostToThread.hpp"

#include <QApplication>
#include <QLoggingCategory>
#include <QString>

namespace chatterino::commands {

using namespace literals;

QString setLoggingRules(const CommandContext &ctx)
{
    if (ctx.words.size() < 2)
    {
        ctx.channel->addSystemMessage(
            "Usage: /c2-set-logging-rules <rules...>. To enable debug logging "
            "for all categories from chatterino, use "
            "'chatterino.*.debug=true'. For the format on the rules, see "
            "https://doc.qt.io/qt-6/"
            "qloggingcategory.html#configuring-categories");
        return {};
    }

    auto filterRules = ctx.words.mid(1).join('\n');

    QLoggingCategory::setFilterRules(filterRules);

    auto message =
        QStringLiteral("Updated filter rules to '%1'.").arg(filterRules);

    if (!qgetenv("QT_LOGGING_RULES").isEmpty())
    {
        message += QStringLiteral(
            " Warning: Logging rules were previously set by the "
            "QT_LOGGING_RULES environment variable. This might cause "
            "interference - see: "
            "https://doc.qt.io/qt-6/qloggingcategory.html#setFilterRules");
    }

    ctx.channel->addSystemMessage(message);
    return {};
}

QString toggleThemeReload(const CommandContext &ctx)
{
    if (getTheme()->isAutoReloading())
    {
        getTheme()->setAutoReload(false);
        ctx.channel->addSystemMessage(u"Disabled theme auto reloading."_s);
        return {};
    }

    getTheme()->setAutoReload(true);
    ctx.channel->addSystemMessage(u"Auto reloading theme every %1 ms."_s.arg(
        Theme::AUTO_RELOAD_INTERVAL_MS));
    return {};
}

QString listEnvironmentVariables(const CommandContext &ctx)
{
    const auto &channel = ctx.channel;
    if (channel == nullptr)
    {
        return "";
    }

    auto env = Env::get();

    QStringList debugMessages{
        "recentMessagesApiUrl: " + env.recentMessagesApiUrl,
        "linkResolverUrl: " + env.linkResolverUrl,
        "proxyUrl: " + env.proxyUrl.value_or("N/A"),
        "twitchServerHost: " + env.twitchServerHost,
        "twitchServerPort: " + QString::number(env.twitchServerPort),
        "twitchServerSecure: " + QString::number(env.twitchServerSecure),
    };

    for (QString &str : debugMessages)
    {
        MessageBuilder builder;
        builder.emplace<TimestampElement>(QTime::currentTime());
        builder.emplace<TextElement>(str, MessageElementFlag::Text,
                                     MessageColor::System);
        channel->addMessage(builder.release(), MessageContext::Original);
    }
    return "";
}

QString listArgs(const CommandContext &ctx)
{
    const auto &channel = ctx.channel;
    if (channel == nullptr)
    {
        return "";
    }

    QString msg = QApplication::instance()->arguments().join(' ');

    channel->addSystemMessage(msg);

    return "";
}

QString forceImageGarbageCollection(const CommandContext &ctx)
{
    (void)ctx;

    runInGuiThread([] {
        auto &iep = ImageExpirationPool::instance();
        iep.freeOld();
    });
    return "";
}

QString forceImageUnload(const CommandContext &ctx)
{
    (void)ctx;

    runInGuiThread([] {
        auto &iep = ImageExpirationPool::instance();
        iep.freeAll();
    });
    return "";
}

QString debugTest(const CommandContext &ctx)
{
    if (!ctx.channel)
    {
        return "";
    }

    const auto command = ctx.words.value(1);

    if (command == "timeout-pubsub")
    {
        QJsonObject data;
        data["created_by_user_id"] = ctx.twitchChannel->roomId();
        data["created_by"] = ctx.twitchChannel->getName();

        BanAction action(data, ctx.twitchChannel->roomId());

        action.source.id = ctx.twitchChannel->roomId();
        action.source.login = ctx.twitchChannel->getName();

        action.target.id = "11148817";
        action.target.login = "pajlada";
        action.duration = 10;

        MessageBuilder msg(action, QDateTime::currentDateTime());
        msg->flags.set(MessageFlag::PubSub);
        ctx.channel->addOrReplaceTimeout(msg.release(),
                                         QDateTime::currentDateTime());
    }
    else if (command == "timeout-irc")
    {
        auto nowMillis = QDateTime::currentDateTime().toSecsSinceEpoch();

        const auto ircText =
            QString(
                "@tmi-sent-ts=%1;room-id=117166826;user-id=11148817;badges=;"
                "badge-info=;flags=;user-type=;emotes=;target-user-id=11148817;"
                "ban-"
                "duration=1 :tmi.twitch.tv CLEARCHAT #testaccount_420 pajlada")
                .arg(nowMillis);
        getApp()->getTwitch()->addFakeMessage(ircText);
    }
    else if (command == "desktop-notify")
    {
        auto title = ctx.twitchChannel->accessStreamStatus()->title;

        getApp()->getToasts()->sendChannelNotification(
            ctx.twitchChannel->getName(), title);
        ctx.channel->addSystemMessage(
            QString("debug-test sent desktop notification"));
    }
    else
    {
        ctx.channel->addSystemMessage(
            QString("debug-test called with command: '%1'").arg(command));
    }

    return "";
}

}  // namespace chatterino::commands
