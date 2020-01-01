#include "common/Env.hpp"

#include <QVariant>

namespace chatterino {

namespace {

    QString readStringEnv(const char *envName, QString defaultValue)
    {
        auto envString = std::getenv(envName);
        if (envString != nullptr)
        {
            return QString(envString);
        }

        return defaultValue;
    }

    uint16_t readPortEnv(const char *envName, uint16_t defaultValue)
    {
        auto envString = std::getenv(envName);
        if (envString != nullptr)
        {
            bool ok;
            auto val = QString(envString).toUShort(&ok);
            if (ok)
            {
                return val;
            }
        }

        return defaultValue;
    }

    uint16_t readBoolEnv(const char *envName, bool defaultValue)
    {
        auto envString = std::getenv(envName);
        if (envString != nullptr)
        {
            return QVariant(QString(envString)).toBool();
        }

        return defaultValue;
    }

}  // namespace

Env::Env()
    : recentMessagesApiUrl(
          readStringEnv("CHATTERINO2_RECENT_MESSAGES_URL",
                        "https://recent-messages.robotty.de/api/v2/"
                        "recent-messages/%1?clearchatToNotice=true"))
    , linkResolverUrl(readStringEnv(
          "CHATTERINO2_LINK_RESOLVER_URL",
          "https://braize.pajlada.com/chatterino/link_resolver/%1"))
    , twitchEmoteSetResolverUrl(readStringEnv(
          "CHATTERINO2_TWITCH_EMOTE_SET_RESOLVER_URL",
          "https://braize.pajlada.com/chatterino/twitchemotes/set/%1/"))
    , imageUploaderUrl(readStringEnv("CHATTERINO2_IMAGE_PASTE_SITE_URL",
                                      "https://i.nuuls.com/upload"))
    , twitchServerHost(
          readStringEnv("CHATTERINO2_TWITCH_SERVER_HOST", "irc.chat.twitch.tv"))
    , twitchServerPort(readPortEnv("CHATTERINO2_TWITCH_SERVER_PORT", 6697))
    , twitchServerSecure(readBoolEnv("CHATTERINO2_TWITCH_SERVER_SECURE", true))
{
}

const Env &Env::get()
{
    static Env instance;
    return instance;
}

}  // namespace chatterino
