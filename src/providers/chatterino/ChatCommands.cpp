#include "ChatCommands.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "common/QLogging.hpp"

namespace chatterino {

void loadChatCommands(
    const QString &channelId,
    std::function<void(std::vector<ExternalChatCommand> &&)> cb)
{
    NetworkRequest("https://api.chatterino.com/chat-commands/twitch/" +
                   channelId)
        .timeout(20000)
        .onSuccess([cb = std::move(cb), channelId](auto result) -> Outcome {
            auto json = result.parseJsonArray();
            auto out = std::vector<ExternalChatCommand>();

            for (auto &&cmdValue : json)
            {
                auto cmd = cmdValue.toObject();

                ExternalChatCommand ex;
                ex.prefix = cmd.value("prefix").toString();
                ex.description = cmd.value("description").toString();
                ex.source = cmd.value("source").toString();

                out.push_back(std::move(ex));
            }

            cb(std::move(out));

            return Success;
        })
        .onError([channelId](NetworkResult result) {
            qCWarning(chatterinoFfzemotes)
                << "Error loading chat commands" << channelId << ", error"
                << result.status();
        })
        .execute();
}

}  // namespace chatterino
