#pragma once

#include "common/FlagsEnum.hpp"
#include "common/UniqueAccess.hpp"
#include "controllers/highlights/HighlightCheck.hpp"
#include "singletons/Settings.hpp"

#include <boost/signals2/connection.hpp>
#include <pajlada/settings.hpp>
#include <pajlada/settings/settinglistener.hpp>
#include <pajlada/signals/signalholder.hpp>
#include <QColor>
#include <QUrl>

#include <cstdint>
#include <utility>
#include <vector>

namespace chatterino {

class TwitchBadge;
struct MessageParseArgs;
class AccountController;
enum class MessageFlag : std::int64_t;
using MessageFlags = FlagsEnum<MessageFlag>;

class HighlightController final
{
public:
    HighlightController(Settings &settings, AccountController *accounts);

    /**
     * @brief Checks the given message parameters if it matches our internal checks, and returns a result
     **/
    [[nodiscard]] std::pair<bool, HighlightResult> check(
        const MessageParseArgs &args,
        const std::vector<TwitchBadge> &twitchBadges, const QString &senderName,
        const QString &originalMessage, const MessageFlags &messageFlags) const;

private:
    /**
     * @brief rebuildChecks is called whenever some outside variable has been changed and our checks need to be updated
     *
     * rebuilds are always full, so if something changes we throw away all checks and build them all up from scratch
     **/
    void rebuildChecks(Settings &settings);

    UniqueAccess<std::vector<HighlightCheck>> checks_;

    pajlada::SettingListener rebuildListener_;
    pajlada::Signals::SignalHolder signalHolder_;
    std::vector<boost::signals2::scoped_connection> bConnections;
};

}  // namespace chatterino
