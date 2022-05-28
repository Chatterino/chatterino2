#pragma once

#include "common/Singleton.hpp"
#include "common/UniqueAccess.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchBadge.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"

#include <QColor>
#include <QUrl>
#include <boost/optional.hpp>

#include <functional>
#include <memory>
#include <utility>

namespace chatterino {

struct HighlightResult {
    HighlightResult(bool _alert, bool _playSound,
                    boost::optional<QUrl> _customSoundUrl,
                    std::shared_ptr<QColor> _color, bool _showInMentions)
        : alert(_alert)
        , playSound(_playSound)
        , customSoundUrl(std::move(_customSoundUrl))
        , color(std::move(_color))
        , showInMentions(_showInMentions)
    {
    }

    static HighlightResult emptyResult()
    {
        return {
            false, false, boost::none, nullptr, false,
        };
    }

    /**
     * @brief alert designates whether this highlight should trigger the taskbar to flash
     **/
    bool alert;

    bool playSound;

    // customSoundUrl may only be set if playSound has been set
    boost::optional<QUrl> customSoundUrl;

    std::shared_ptr<QColor> color;

    bool showInMentions{false};

    bool operator==(const HighlightResult &other) const
    {
        if (this->alert != other.alert)
        {
            return false;
        }
        if (this->playSound != other.playSound)
        {
            return false;
        }
        if (this->customSoundUrl != other.customSoundUrl)
        {
            return false;
        }

        if (this->color && other.color)
        {
            if (*this->color != *other.color)
            {
                return false;
            }
        }

        if (this->showInMentions != other.showInMentions)
        {
            return false;
        }

        return true;
    }

    bool operator!=(const HighlightResult &other) const
    {
        return !(*this == other);
    }

    [[nodiscard]] bool empty() const
    {
        return !this->alert && !this->playSound &&
               !this->customSoundUrl.has_value() && !this->color &&
               !this->showInMentions;
    }

    [[nodiscard]] bool full() const
    {
        return this->alert && this->playSound &&
               this->customSoundUrl.has_value() && this->color &&
               this->showInMentions;
    }
};

struct HighlightCheck {
    using Checker = std::function<boost::optional<HighlightResult>(
        const MessageParseArgs &args, const std::vector<Badge> &badges,
        const QString &senderName, const QString &originalMessage)>;
    Checker cb;
};

class HighlightController final : public Singleton
{
public:
    void initialize(Settings &settings, Paths &paths) override;

    /**
     * @brief Checks the given message parameters if it matches our internal checks, and returns a result
     **/
    [[nodiscard]] std::pair<bool, HighlightResult> check(
        const MessageParseArgs &args, const std::vector<Badge> &badges,
        const QString &senderName, const QString &originalMessage) const;

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
};

}  // namespace chatterino
