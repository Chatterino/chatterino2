#include "common/Env.hpp"

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
{
}

const Env &Env::get()
{
    static Env instance;
    return instance;
}

}  // namespace chatterino
