#pragma once

#include <QString>

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

struct PerformChannelAction {
    QString channelID;
    QString rawTarget;
    QString reason;
};

std::vector<PerformChannelAction> parseBanCommand(const CommandContext &ctx);

/// /ban
QString sendBan(const CommandContext &ctx);
/// /banid
QString sendBanById(const CommandContext &ctx);

/// /timeout
QString sendTimeout(const CommandContext &ctx);

}  // namespace chatterino::commands
