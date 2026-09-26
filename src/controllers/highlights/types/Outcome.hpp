// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/highlights/HighlightResult.hpp"
#include "controllers/highlights/types/Concepts.hpp"

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
    template <typename H>
    static Outcome create()
    {
        if constexpr (HasDefaultSound<H>)
        {
            return Outcome(H::BACKGROUND_COLOR_DEFAULT, H::SOUND_DEFAULT);
        }
        return Outcome(H::BACKGROUND_COLOR_DEFAULT);
    }

    template <typename H>
    HighlightResult makeSimpleResult() const
    {
        return HighlightResult{
            .alert = this->alert.value_or(H::ALERT_DEFAULT),
            .sound = this->getSoundURL(),
            .color = this->getBackgroundColor(),
            .showInMentions =
                this->showInMentions.value_or(H::SHOW_IN_MENTIONS_DEFAULT),
        };
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

    /// Returns the URL that should actually be played when a highlight is triggered, or invalid/empty if no sound should be played.
    QUrl getSoundURL() const;

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

    bool operator==(const Outcome &other) const = default;

    void serialize(rapidjson::Value &ret,
                   rapidjson::Document::AllocatorType &a) const;

    bool deserialize(const rapidjson::Value &value);

    friend QDebug operator<<(QDebug dbg, const Outcome &v);

private:
    explicit Outcome(const QColor &defaultBackgroundColor_,
                     QStringView defaultSound_ = {})
        : defaultBackgroundColor(defaultBackgroundColor_)
        , defaultSound(defaultSound_)
    {
        *this->resolvedBackgroundColor = this->defaultBackgroundColor;
    }

    std::shared_ptr<QColor> resolvedBackgroundColor =
        std::make_shared<QColor>();

    void updateSoundURL();

    /// Contains the URL that should actually be played when a highlight is triggered, or invalid/empty if no sound should be played.
    /// Transient. Not stored as-is in the JSON.
    QUrl soundURL;

    // This should be set during initialization
    QColor defaultBackgroundColor;

    // This should be set during initialization
    QStringView defaultSound;

    /// The background color to apply to the message.
    /// If the pointer is unset, use the highlight's default color
    /// If the pointer is valid, but the QColor is invalid: Don't apply a background color
    std::optional<QColor> backgroundColor;
};

}  // namespace chatterino::highlights
