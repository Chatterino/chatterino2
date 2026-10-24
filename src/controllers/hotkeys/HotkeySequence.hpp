// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QKeySequence>
#include <QString>
#include <Qt>

#include <optional>

namespace chatterino {

/// True for extra buttons (Mouse Back/Forward and Mouse Button 6+).
/// Left, Right, and Middle are not extra buttons.
[[nodiscard]] bool isExtraMouseButton(Qt::MouseButton button);

/// Physical button number (4 = Back, 5 = Forward, …) for an extra mouse button.
[[nodiscard]] std::optional<int> extraMouseButtonNumber(Qt::MouseButton button);

/// Extra mouse button for a physical button number (4–27).
[[nodiscard]] std::optional<Qt::MouseButton> extraMouseButtonFromNumber(
    int number);

/**
 * Keyboard chord or a single extra mouse button.
 *
 * Mouse bindings never include keyboard modifiers.
 */
class HotkeySequence
{
public:
    HotkeySequence() = default;
    HotkeySequence(const QKeySequence &keySequence);
    explicit HotkeySequence(Qt::MouseButton mouseButton);

    [[nodiscard]] static HotkeySequence fromPortableString(const QString &text);

    [[nodiscard]] bool isEmpty() const;
    [[nodiscard]] bool isMouse() const;

    [[nodiscard]] QKeySequence keySequence() const;
    [[nodiscard]] Qt::MouseButton mouseButton() const;

    /// OS-specific text for the GUI (e.g. "Ctrl+F5" or "Mouse Back").
    [[nodiscard]] QString toString() const;

    /// Portable text for settings (e.g. "Ctrl+F5" or "MouseBack").
    [[nodiscard]] QString toPortableString() const;

    bool operator==(const HotkeySequence &other) const;
    bool operator!=(const HotkeySequence &other) const;

private:
    QKeySequence keySequence_;
    Qt::MouseButton mouseButton_ = Qt::NoButton;
};

}  // namespace chatterino
