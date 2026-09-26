// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>
#include <QStringView>

#include <concepts>

namespace chatterino::highlights {

template <typename T>
concept HasDynamicID = requires(T a) {
    { a.getID() } -> std::same_as<QStringView>;
};

template <typename T>
concept HasDynamicDefaultName = requires(T a) {
    { a.getDefaultName() } -> std::same_as<QString>;
};

template <typename T>
concept HasCustomizableName = requires(T a) {
    { a.name } -> std::convertible_to<QString>;
};

template <typename T>
concept HasDynamicAndCustomizableName =
    HasDynamicDefaultName<T> && HasCustomizableName<T>;

template <typename T>
concept SupportsErrors = requires(T a) {
    { a.getError() } -> std::same_as<QString>;
};

template <typename T>
concept HasDefaultSound = requires {
    { T::SOUND_DEFAULT } -> std::convertible_to<QStringView>;
};

template <typename T>
concept SupportsCaseSensitivity = requires(T a) {
    { a.isCaseSensitive() } -> std::same_as<bool>;
    { a.setCaseSensitive(true) };
};

template <typename T>
concept SupportsRegex = requires(T a) {
    { a.isRegex() } -> std::same_as<bool>;
    { a.setRegex(true) };
};

template <typename T>
concept SupportsDefaultName = requires(T a) {
    { a.getDefaultName() } -> std::same_as<QString>;
};

template <typename T>
concept SupportsGetID = requires(T a) {
    { a.getID() } -> std::same_as<QStringView>;
};

template <typename T>
concept HasDescription = requires {
    { T::DESCRIPTION } -> std::convertible_to<QStringView>;
};

}  // namespace chatterino::highlights
