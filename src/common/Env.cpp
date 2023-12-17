#include "common/Env.hpp"

#include "common/QLogging.hpp"
#include "util/TypeName.hpp"

#include <QtGlobal>
#include <QVariant>

namespace chatterino {

namespace {

    template <typename T>
    void warn(const char *envName, const QString &envString, T defaultValue)
    {
        const auto typeName = QString::fromStdString(
            std::string(type_name<decltype(defaultValue)>()));

        qCWarning(chatterinoEnv).noquote()
            << QStringLiteral(
                   "Cannot parse value '%1' of environment variable '%2' "
                   "as a %3, reverting to default value '%4'")
                   .arg(envString)
                   .arg(envName)
                   .arg(typeName)
                   .arg(defaultValue);
    }

    std::optional<QString> readOptionalStringEnv(const char *envName)
    {
        auto envString = qEnvironmentVariable(envName);
        if (!envString.isEmpty())
        {
            return envString;
        }

        return std::nullopt;
    }

    uint16_t readPortEnv(const char *envName, uint16_t defaultValue)
    {
        auto envString = qEnvironmentVariable(envName);
        if (!envString.isEmpty())
        {
            bool ok = false;
            auto val = envString.toUShort(&ok);
            if (ok)
            {
                return val;
            }

            warn(envName, envString, defaultValue);
        }

        return defaultValue;
    }

    bool readBoolEnv(const char *envName, bool defaultValue)
    {
        auto envString = qEnvironmentVariable(envName);
        if (!envString.isEmpty())
        {
            return QVariant(envString).toBool();
        }

        return defaultValue;
    }

}  // namespace

Env::Env()
    : recentMessagesApiUrl(
          qEnvironmentVariable("CHATTERINO2_RECENT_MESSAGES_URL",
                               "https://recent-messages.robotty.de/api/v2/"
                               "recent-messages/%1"))
    , linkResolverUrl(qEnvironmentVariable(
          "CHATTERINO2_LINK_RESOLVER_URL",
          "https://braize.pajlada.com/chatterino/link_resolver/%1"))
    , twitchServerHost(qEnvironmentVariable("CHATTERINO2_TWITCH_SERVER_HOST",
                                            "irc.chat.twitch.tv"))
    , twitchServerPort(readPortEnv("CHATTERINO2_TWITCH_SERVER_PORT", 443))
    , twitchServerSecure(readBoolEnv("CHATTERINO2_TWITCH_SERVER_SECURE", true))
    , proxyUrl(readOptionalStringEnv("CHATTERINO2_PROXY_URL"))
{
}

const Env &Env::get()
{
    static Env instance;
    return instance;
}

}  // namespace chatterino
