#include "controllers/ignores/IgnoreController.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "singletons/Settings.hpp"

namespace chatterino {

bool isIgnoredMessage(IgnoredMessageParameters &&params)
{
    if (!params.message.isEmpty())
    {
        // TODO(pajlada): Do we need to check if the phrase is valid first?
        auto phrases = getSettings()->ignoredMessages.readOnly();
        for (const auto &phrase : *phrases)
        {
            if (phrase.isBlock() && phrase.isMatch(params.message))
            {
                qCDebug(chatterinoMessage)
                    << "Blocking message because it contains ignored phrase"
                    << phrase.getPattern();
                return true;
            }
        }
    }

    if (!params.twitchUserID.isEmpty() &&
        getSettings()->enableTwitchBlockedUsers)
    {
        auto sourceUserID = params.twitchUserID;

        bool isBlocked = getIApp()
                             ->getAccounts()
                             ->twitch.getCurrent()
                             ->blockedUserIds()
                             .contains(sourceUserID);
        if (isBlocked)
        {
            switch (static_cast<ShowIgnoredUsersMessages>(
                getSettings()->showBlockedUsersMessages.getValue()))
            {
                case ShowIgnoredUsersMessages::IfModerator:
                    if (params.isMod || params.isBroadcaster)
                    {
                        return false;
                    }
                    break;
                case ShowIgnoredUsersMessages::IfBroadcaster:
                    if (params.isBroadcaster)
                    {
                        return false;
                    }
                    break;
                case ShowIgnoredUsersMessages::Never:
                    break;
            }

            return true;
        }
    }

    return false;
}

}  // namespace chatterino
