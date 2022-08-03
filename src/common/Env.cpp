#include "common/Env.hpp"

#include "common/QLogging.hpp"
#include "util/TypeName.hpp"

#include <QVariant>

namespace chatterino {

namespace {

    template <typename T>
    void warn(const char *envName, T defaultValue)
    {
        auto *envString = std::getenv(envName);
        if (!envString)
        {
            // This function is not supposed to be used for non-existant
            // environment variables.
            return;
        }

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
            else
            {
                warn(envName, defaultValue);
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

    uint32_t readUint32Env(const char *envName, uint32_t defaultValue)
    {
        const auto *envString = std::getenv(envName);
        if (envString != nullptr)
        {
            bool ok;
            const auto val = QString(envString).toUInt(&ok);
            if (ok)
            {
                return val;
            }
            else
            {
                warn(envName, defaultValue);
            }
        }

        return defaultValue;
    }

}  // namespace

Env::Env()
    : recentMessages{readStringEnv("CHATTERINO2_RECENT_MESSAGES_URL",
                                   "https://recent-messages.robotty.de/api/v2/"
                                   "recent-messages/%1"),
                     // TODO(leon): Figure out reasonable values for these.
                     readUint32Env(
                         "CHATTERINO2_RECENT_MESSAGES_RATE_LIMIT_BUDGET", 10),
                     readUint32Env(
                         "CHATTERINO2_RECENT_MESSAGES_RATE_LIMIT_COOLDOWN",
                         1000)}
    , linkResolverUrl(readStringEnv(
          "CHATTERINO2_LINK_RESOLVER_URL",
          "https://braize.pajlada.com/chatterino/link_resolver/%1"))
    , twitchServerHost(
          readStringEnv("CHATTERINO2_TWITCH_SERVER_HOST", "irc.chat.twitch.tv"))
    , twitchServerPort(readPortEnv("CHATTERINO2_TWITCH_SERVER_PORT", 443))
    , twitchServerSecure(readBoolEnv("CHATTERINO2_TWITCH_SERVER_SECURE", true))
{
}

const Env &Env::get()
{
    static Env instance;
    return instance;
}

}  // namespace chatterino
