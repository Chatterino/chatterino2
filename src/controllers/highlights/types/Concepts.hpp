// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>
#include <QStringView>

#include <concepts>

namespace chatterino::highlights {

/// Checks if the given highlight has a dynamic ID (i.e. if it's user-defined).
template <typename T>
concept HasDynamicID = requires(T a) {
    { a.getID() } -> std::same_as<QStringView>;
};

/// Checks if the given highlight can generate a default name based on the highlight's configuration.
template <typename T>
concept HasDynamicDefaultName = requires(T a) {
    { a.getDefaultName() } -> std::same_as<QString>;
};

/// Checks if the given highlight can generate errors based on its configuration.
template <typename T>
concept SupportsErrors = requires(T a) {
    { a.getError() } -> std::same_as<QString>;
};

/// Checks if the given highlight has a sound that should be played by default if it is triggered and the user has not explicitly disabled the sound.
template <typename T>
concept HasDefaultSound = requires {
    { T::SOUND_DEFAULT } -> std::convertible_to<QStringView>;
};

/// Checks if the given highlight supports configuring its pattern's case sensitivity.
template <typename T>
concept SupportsCaseSensitivity = requires(T a) {
    { a.isCaseSensitive() } -> std::same_as<bool>;
    { a.setCaseSensitive(true) };
};

/// Checks if the given highlight supports configuring whether its pattern should be parsed as a regular expression.
template <typename T>
concept SupportsRegex = requires(T a) {
    { a.isRegex() } -> std::same_as<bool>;
    { a.setRegex(true) };
};

/// Checks if the given highlight has an extra description that should be shown to the user when the highlight is created or edited.
template <typename T>
concept HasDescription = requires {
    { T::DESCRIPTION } -> std::convertible_to<QStringView>;
};

}  // namespace chatterino::highlights
