// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QColor>
#include <QDebug>
#include <QString>
#include <QStringView>
#include <QUrl>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <cassert>
#include <memory>
#include <optional>

namespace chatterino::highlights {

struct Outcome {
    explicit Outcome(const QColor &defaultBackgroundColor_,
                     QStringView defaultSound_ = {})
        : defaultBackgroundColor(defaultBackgroundColor_)
        , defaultSound(defaultSound_)
    {
        *this->resolvedBackgroundColor = this->defaultBackgroundColor;
    }

    /// Whether to add the matching message to the /mentions channel
    std::optional<bool> showInMentions;

    /// Show an OS-specific alert.
    /// On Windows, this will flash Chatterino in the taskbar.
    /// On macOS, this will make Chatterino bounce in the taskbar.
    std::optional<bool> alert;

    /// The user-configured string for of the sound that will be played.
    /// A null sound means whatever the highlight-controlled default sound will be played.
    /// An empty string means no sound is played.
    /// A string matching one of our built-in sounds (e.g. "001-ping2") means it will play the given resource.
    /// An absolute file URL, probably prefixed with file:/// means it will play the custom sound at that path.
    QString sound;

    void setSound(const QString &newSound);

    /// Contains the URL that should actually be played when a highlight is triggered, or invalid/empty if no sound should be played.
    /// Transient. Not stored as-is in the JSON.
    QUrl soundURL;

    /*
    std::optional<int> volume;
    */

    void setBackgroundColor(const std::optional<QColor> &color)
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

    // This should be set during initialization
    QStringView defaultSound;

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
