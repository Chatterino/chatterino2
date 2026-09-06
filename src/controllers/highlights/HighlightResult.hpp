// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QColor>
#include <QStringList>
#include <QUrl>

#include <memory>
#include <optional>
#include <ostream>

namespace chatterino {

struct HighlightResult {
    /**
     * @brief Construct an empty HighlightResult with all side-effects disabled
     **/
    static HighlightResult emptyResult();

    // XXX TODO TEMP TEMPORARY ID OF HIGHLIGHT THAT TRIGGERED THE HIGHLIGHT :-)
    QStringList ids;

    /**
     * @brief true if highlight should trigger the taskbar to flash
     **/
    bool alert{false};

    /// Sound to play
    /// Empty or invalid = no sound should play
    QUrl sound;

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
