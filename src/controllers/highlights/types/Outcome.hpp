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

    /// Play a sound when this highlight is triggered.
    /// A null sound means whatever the default sound is should be played (controlled by the highlight)
    /// An empty string means no sound is played
    /// A string matching one of our built-in sounds means it will play the given resource
    /// An absolute file URL, probably prefixed with file:/// meaning it will play a custom sound
    QString sound;

    void setSound(const QString &newSound);

    /// Transient. Not stored as-is in the JSON.
    QUrl soundURL;

    QUrl getSoundURLWithDefault(const QStringView &defaultSound) const
    {
        if (this->sound.isNull())
        {
            return QUrl{defaultSound.toString()};
        }

        return this->soundURL;
    }

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

    void updateSoundURL();
};

}  // namespace chatterino::highlights
