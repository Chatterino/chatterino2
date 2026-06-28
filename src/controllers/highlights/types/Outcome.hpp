// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QColor>
#include <QDebug>
#include <QString>
#include <QUrl>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <cassert>
#include <memory>
#include <optional>

namespace chatterino::highlights {

struct Outcome {
    explicit Outcome(const QColor &defaultBackgroundColor_)
        : defaultBackgroundColor(defaultBackgroundColor_)
    {
        *this->resolvedBackgroundColor = this->defaultBackgroundColor;
    }

    /// Whether to add the matching message to the /mentions channel
    std::optional<bool> showInMentions;

    /// Show an OS-specific alert.
    /// On Windows, this will flash Chatterino in the taskbar.
    /// On macOS, this will make Chatterino bounce in the taskbar.
    std::optional<bool> alert;

    /// Play a sound.
    /// If the highlight specifies a "customSoundURL", it will play that, otherwise it will
    /// play the default highlight sound.
    std::optional<bool> playSound;

    /// The custom sound URL to play if playSound is enabled.
    QUrl customSoundURL;

    void setBackgroundColor(std::optional<QColor> color)
    {
        this->backgroundColor = color;

        if (this->backgroundColor)
        {
            *this->resolvedBackgroundColor = *this->backgroundColor;
        }
        else
        {
            *this->resolvedBackgroundColor = this->defaultBackgroundColor;
        }
    }

    std::shared_ptr<QColor> getBackgroundColor() const
    {
        if (this->backgroundColor.has_value())
        {
            assert(this->resolvedBackgroundColor);
            assert(*this->resolvedBackgroundColor == *this->backgroundColor);

            return this->resolvedBackgroundColor;
        }

        return this->resolvedBackgroundColor;
    }

    // This should be set during initialization
    QColor defaultBackgroundColor;

    /// The background color to apply to the message.
    /// If the pointer is unset, use the highlight's default color
    /// If the pointer is valid, but the QColor is invalid: Don't apply a background color
    std::optional<QColor> backgroundColor;

    std::shared_ptr<QColor> getBackgroundColorWithDefault(
        const QColor &defaultColor) const;

    bool operator==(const Outcome &other) const = default;

    void serialize(rapidjson::Value &ret,
                   rapidjson::Document::AllocatorType &a) const;

    bool deserialize(const rapidjson::Value &value);

    friend QDebug operator<<(QDebug dbg, const Outcome &v);

private:
    std::shared_ptr<QColor> resolvedBackgroundColor =
        std::make_shared<QColor>();
};

}  // namespace chatterino::highlights
