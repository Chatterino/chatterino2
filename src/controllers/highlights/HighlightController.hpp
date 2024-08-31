#pragma once

#include "common/UniqueAccess.hpp"
#include "messages/MessageFlag.hpp"
#include "singletons/Settings.hpp"

#include <boost/signals2/connection.hpp>
#include <pajlada/settings.hpp>
#include <pajlada/settings/settinglistener.hpp>
#include <QColor>
#include <QUrl>

#include <functional>
#include <memory>
#include <optional>
#include <utility>

namespace chatterino {

class Badge;
struct MessageParseArgs;
class AccountController;

struct HighlightResult {
    HighlightResult(bool _alert, bool _playSound,
                    std::optional<QUrl> _customSoundUrl,
                    std::shared_ptr<QColor> _color, bool _showInMentions);

    /**
     * @brief Construct an empty HighlightResult with all side-effects disabled
     **/
    static HighlightResult emptyResult();

    /**
     * @brief true if highlight should trigger the taskbar to flash
     **/
    bool alert{false};

    /**
     * @brief true if highlight should play a notification sound
     **/
    bool playSound{false};

    /**
     * @brief Can be set to a different sound that should play when this highlight is activated
     *
     * May only be set if playSound is true
     **/
    std::optional<QUrl> customSoundUrl{};

    /**
     * @brief set if highlight should set a background color
     **/
    std::shared_ptr<QColor> color{};

    /**
     * @brief true if highlight should show message in the /mentions split
     **/
    bool showInMentions{false};

    bool operator==(const HighlightResult &other) const;
    bool operator!=(const HighlightResult &other) const;

    /**
     * @brief Returns true if no side-effect has been enabled
     **/
    [[nodiscard]] bool empty() const;

    /**
     * @brief Returns true if all side-effects have been enabled
     **/
    [[nodiscard]] bool full() const;

    friend std::ostream &operator<<(std::ostream &os,
                                    const HighlightResult &result);
};

struct HighlightCheck {
    using Checker = std::function<std::optional<HighlightResult>(
        const MessageParseArgs &args, const std::vector<Badge> &badges,
        const QString &senderName, const QString &originalMessage,
        const MessageFlags &messageFlags, bool self)>;
    Checker cb;
};

class HighlightController final
{
public:
    HighlightController(Settings &settings, AccountController *accounts);

    /**
     * @brief Checks the given message parameters if it matches our internal checks, and returns a result
     **/
    [[nodiscard]] std::pair<bool, HighlightResult> check(
        const MessageParseArgs &args, const std::vector<Badge> &badges,
        const QString &senderName, const QString &originalMessage,
        const MessageFlags &messageFlags) const;

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
