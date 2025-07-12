#pragma once

#include "messages/MessageColor.hpp"

#include <QString>

#include <optional>

class QColor;

namespace chatterino {

class IUserDataController;
class ChannelChatters;

namespace twitch {

struct GetUserColorParams {
    QString userLogin;
    QString userID;

    const IUserDataController *userDataController{};
    const ChannelChatters *channelChatters{};

    QColor color;
};

/// Attempt to get the color of the user based on the provided parameters.
///
/// If there's a valid color for the given user ID in the user data controller, return it.
/// If there's a valid `color` parameter, return it.
/// If there's a valid color for the given user login in the channel chatters set, return it.
/// Otherwise, return nullopt.
std::optional<MessageColor> getUserColor(const GetUserColorParams &params);

}  // namespace twitch
}  // namespace chatterino
