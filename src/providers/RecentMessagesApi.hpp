#pragma once

#include "ForwardDecl.hpp"
#include "util/RatelimitBucket.hpp"

#include <QString>

#include <functional>
#include <memory>
#include <vector>

namespace chatterino {

class RecentMessagesApi
{
public:
    using ResultCallback = std::function<void(const std::vector<MessagePtr> &)>;
    using ErrorCallback = std::function<void()>;

    static RecentMessagesApi &instance();

    RecentMessagesApi(const RecentMessagesApi &original) = delete;
    RecentMessagesApi(RecentMessagesApi &&other) = delete;
    RecentMessagesApi &operator=(const RecentMessagesApi &original) = delete;
    RecentMessagesApi &operator=(RecentMessagesApi &&other) = delete;

    /**
     * @brief Loads recent messages for a channel using the Recent Messages API
     * 
     * @param channelName Name of Twitch channel
     * @param channelPtr Weak pointer to Channel to use to build messages
     * @param onLoaded Callback taking the built messages as a const std::vector<MessagePtr> &
     * @param onError Callback called when the network request fails
     */
    void loadRecentMessages(const QString &channelName,
                            std::weak_ptr<Channel> channelPtr,
                            ResultCallback onLoaded, ErrorCallback onError);

private:
    RecentMessagesApi();

    struct RequestContext {
        QString channelName;
        std::weak_ptr<Channel> channelPtr;
        ResultCallback onLoaded;
        ErrorCallback onError;
    };

    static void doMessageLoad(const RequestContext &ctx);

    RatelimitBucket<RequestContext> requestBucket_;
};

}  // namespace chatterino
