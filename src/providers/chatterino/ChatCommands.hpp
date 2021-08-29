#pragma once

#include <functional>
#include <memory>

namespace chatterino {

class Channel;

struct ExternalChatCommand {
    QString prefix;
    QString description;
    QString source;
};

void loadChatCommands(
    const QString &channelId,
    std::function<void(std::vector<ExternalChatCommand> &&)> cb);

}  // namespace chatterino
