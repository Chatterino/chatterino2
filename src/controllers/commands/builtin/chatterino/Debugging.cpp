#include "controllers/commands/builtin/chatterino/Debugging.hpp"

#include "Application.hpp"
#include "common/Channel.hpp"
#include "common/Env.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "messages/Image.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "messages/MessageElement.hpp"
#include "providers/twitch/eventsub/Controller.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/Toasts.hpp"
#include "singletons/Updates.hpp"
#include "singletons/WindowManager.hpp"
#include "util/PostToThread.hpp"

#include <QApplication>
#include <QLoggingCategory>
#include <QString>

using namespace Qt::StringLiterals;

namespace chatterino::commands {

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

QString forceLayoutChannelViews(const CommandContext & /*ctx*/)
{
    getApp()->getWindows()->forceLayoutChannelViews();
    return {};
}

QString incrementImageGeneration(const CommandContext & /*ctx*/)
{
    getApp()->getWindows()->incGeneration();
    return {};
}

QString invalidateBuffers(const CommandContext & /*ctx*/)
{
    getApp()->getWindows()->invalidateChannelViewBuffers();
    return {};
}

QString eventsub(const CommandContext & /*ctx*/)
{
    getApp()->getEventSub()->debug();
    return {};
}

QString debugTest(const CommandContext &ctx)
{
    if (!ctx.channel)
    {
        return "";
    }

    const auto command = ctx.words.value(1);

    if (command == "timeout-irc")
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
    else if (command == "update-check")
    {
        getApp()->getUpdates().checkForUpdates();
        ctx.channel->addSystemMessage(QString("checking for updates"));
    }
    else if (command == "save-settings")
    {
        ctx.channel->addSystemMessage(u"requesting settings save"_s);
        auto res = getSettings()->requestSave();
        switch (res)
        {
            case pajlada::Settings::SettingManager::SaveResult::Failed:
                ctx.channel->addSystemMessage(u"setting save failed"_s);
                break;
            case pajlada::Settings::SettingManager::SaveResult::Success:
                ctx.channel->addSystemMessage(u"setting save success"_s);
                break;
            case pajlada::Settings::SettingManager::SaveResult::Skipped:
                ctx.channel->addSystemMessage(u"setting save skipped"_s);
                break;
        }
    }
    else if (command == "shared-chat-badge-messages")
    {
        const auto nowMillis = QDateTime::currentDateTime().toSecsSinceEpoch();
        const std::vector messages =  {
            QString("@tmi-sent-ts=%1;id=3e0750f7-541e-4f7c-8fec-4f943bfd84a3;room-id=11148817;user-id=123706702;display-name=fastTV;badges=bits/100;badge-info=;color=#00FF7F;flags=5-8:P.3;user-type=;emotes=;source-only=0;source-badge-info=subscriber/52;source-id=0dd9957e-6140-498f-a6e8-e62d1b43dfe7;source-room-id=22484632;source-badges=moderator/1,subscriber/48 :fasttv!fasttv@fasttv.tmi.twitch.tv PRIVMSG #pajlada :I'm a mod in a shared chat").arg(nowMillis),
            QString("@tmi-sent-ts=%1;mod=1;id=d37d2177-2df0-4da2-bdac-34f6b4272818;room-id=11148817;user-id=50931717;display-name=BlackipinoGambino;badges=moderator/1,twitch-recap-2025/1;badge-info=;color=#1E90FF;flags=;user-type=mod;emotes=;source-only=0;source-room-id=22484632;source-id=b11aa5aa-b50c-4205-9e91-799ca4c4b587;source-badge-info=subscriber/76;source-badges=moderator/1,subscriber/72,twitch-recap-2025/1 :blackipinogambino!blackipinogambino@blackipinogambino.tmi.twitch.tv PRIVMSG #pajlada :I'm a mod in both chats").arg(nowMillis),
            QString("@tmi-sent-ts=%1;id=5956b5b5-9f40-4c98-a10b-610141aed8f3;room-id=11148817;user-id=232174108;client-nonce=61df60966afb055c98085e728fba667c;display-name=fire_val;badges=partner/1;badge-info=;color=#FF0000;flags=;user-type=;emotes=;source-room-id=22484632;source-badge-info=subscriber/9;source-badges=vip/1,subscriber/9,partner/1;source-id=8fb99f1e-c302-49ea-8cc6-626a4632b937;source-only=0 :fire_val!fire_val@fire_val.tmi.twitch.tv PRIVMSG #pajlada :I'm a VIP in a shared chat").arg(nowMillis),
            QString("@tmi-sent-ts=%1;vip=1;id=936263ef-8588-4435-9bb7-7a0836046d72;room-id=11148817;user-id=112423840;client-nonce=e516e4e906d9ab239230a00bd1f22e20;display-name=TrulyTenzin;badges=vip/1,partner/1;badge-info=;color=#FF5656;flags=;user-type=;emotes=;source-id=9ff662e2-4bbc-42b5-a616-007b25078afa;source-badge-info=subscriber/52;source-room-id=22484632;source-badges=vip/1,subscriber/48,partner/1;source-only=0 :trulytenzin!trulytenzin@trulytenzin.tmi.twitch.tv PRIVMSG #pajlada :I'm a VIP in both chats").arg(nowMillis)
        };

        for (const auto &msg : messages)
        {
            getApp()->getTwitch()->addFakeMessage(msg);
        }
    }
    else
    {
        ctx.channel->addSystemMessage(
            QString("debug-test called with command: '%1'").arg(command));
    }

    return "";
}

}  // namespace chatterino::commands
