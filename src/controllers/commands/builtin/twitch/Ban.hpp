#pragma once

#include <nonstd/expected.hpp>
#include <QString>

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

struct PerformChannelAction {
    QString channelID;
    QString rawTarget;
    QString reason;
    int duration;
};

nonstd::expected<std::vector<PerformChannelAction>, QString> parseChannelAction(
    const CommandContext &ctx, const QString &command, const QString &usage,
    bool withDuration);

/// /ban
QString sendBan(const CommandContext &ctx);
/// /banid
QString sendBanById(const CommandContext &ctx);

/// /timeout
QString sendTimeout(const CommandContext &ctx);

}  // namespace chatterino::commands
