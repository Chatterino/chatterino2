#pragma once

#include "providers/twitch/TwitchChannel.hpp"

#include <QString>

#include <functional>

namespace chatterino {

template <typename... T>
using ResultCallback = std::function<void(T...)>;
using ErrorCallback = std::function<void(QString error)>;

class ChattersApi
{
public:
    /**
     * @brief Loads chatters for a Twitch channel using Chatters API
     *
     * @param channel Twitch channel to load chatters for
     * @param onLoaded Callback for successful requests
     * @param onError Callback for failed requests
     */
    static void loadChatters(TwitchChannel *channel,
                             ResultCallback<QString, QStringList,
                                 QStringList, QStringList> onLoaded,
                             ErrorCallback onError);

    /**
     * @brief Loads moderators for a Twitch channel using Chatters API
     *
     * @param channel Twitch channel to load moderators for
     * @param onLoaded Callback for successful requests
     * @param onError Callback for failed requests
     */
    static void loadModerators(TwitchChannel *channel,
                               ResultCallback<QStringList> onLoaded,
                               ErrorCallback onError);

    /**
     * @brief Loads vips for a Twitch channel using Chatters API
     *
     * @param channel Twitch channel to load vips for
     * @param onLoaded Callback for successful requests
     * @param onError Callback for failed requests
     */
    static void loadVips(TwitchChannel *channel,
                         ResultCallback<QStringList> onLoaded,
                         ErrorCallback onError);
};

}  // namespace chatterino
