#include "providers/twitch/UserColor.hpp"

#include "common/ChannelChatters.hpp"
#include "controllers/userdata/UserDataController.hpp"
#include "messages/MessageColor.hpp"

namespace chatterino::twitch {

std::optional<MessageColor> getUserColor(const GetUserColorParams &params)
{
    if (params.userDataController != nullptr)
    {
        if (!params.userID.isEmpty())
        {
            if (const auto &oUser =
                    params.userDataController->getUser(params.userID))
            {
                const auto &user = *oUser;
                if (user.color && user.color->isValid())
                {
                    return {*user.color};
                }
            }
        }
    }

    if (params.color.isValid())
    {
        return {params.color};
    }

    if (params.channelChatters != nullptr)
    {
        if (!params.userLogin.isEmpty())
        {
            if (auto color =
                    params.channelChatters->getUserColor(params.userLogin);
                color.isValid())
            {
                return {color};
            }
        }
    }

    return std::nullopt;
}

}  // namespace chatterino::twitch
