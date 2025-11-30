#pragma once

#include <QColor>
#include <QUrl>

#include <memory>
#include <optional>
#include <ostream>

namespace chatterino {

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

}  // namespace chatterino
