#pragma once

#include <nonstd/expected.hpp>
#include <QString>

#include <ostream>
#include <tuple>
#include <vector>

namespace chatterino {

struct CommandContext;
struct HelixUser;

}  // namespace chatterino

namespace chatterino::commands {

struct IncompleteHelixUser {
    QString id;
    QString login;
    QString displayName;

    bool hydrateFrom(const std::vector<HelixUser> &users);

    bool operator==(const IncompleteHelixUser &other) const
    {
        return std::tie(this->id, this->login, this->displayName) ==
               std::tie(other.id, other.login, other.displayName);
    }
};

struct PerformChannelAction {
    // Channel to perform the action in
    IncompleteHelixUser channel;
    // Target to perform the action on
    IncompleteHelixUser target;
    QString reason;
    int duration{};

    bool operator==(const PerformChannelAction &other) const
    {
        return std::tie(this->channel, this->target, this->reason,
                        this->duration) == std::tie(other.channel, other.target,
                                                    other.reason,
                                                    other.duration);
    }
};

std::ostream &operator<<(std::ostream &os, const IncompleteHelixUser &u);
// gtest printer
// NOLINTNEXTLINE(readability-identifier-naming)
void PrintTo(const PerformChannelAction &a, std::ostream *os);

nonstd::expected<std::vector<PerformChannelAction>, QString> parseChannelAction(
    const CommandContext &ctx, const QString &command, const QString &usage,
    bool withDuration, bool withReason);

}  // namespace chatterino::commands
