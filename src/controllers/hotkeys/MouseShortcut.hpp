// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <Qt>

#include <functional>

class QWidget;

namespace chatterino {

/**
 * Like QShortcut, but for extra mouse buttons (Back/Forward and button 6+).
 *
 * Keyboard modifiers are ignored. The shortcut is active according to
 * Qt::ShortcutContext, matching QShortcut.
 */
class MouseShortcut : public QObject
{
    Q_OBJECT

public:
    MouseShortcut(Qt::MouseButton button, Qt::ShortcutContext context,
                  QWidget *parent);
    ~MouseShortcut() override;

    MouseShortcut(const MouseShortcut &) = delete;
    MouseShortcut(MouseShortcut &&) = delete;
    MouseShortcut &operator=(const MouseShortcut &) = delete;
    MouseShortcut &operator=(MouseShortcut &&) = delete;

    [[nodiscard]] Qt::MouseButton button() const;
    [[nodiscard]] Qt::ShortcutContext context() const;
    [[nodiscard]] QWidget *parentWidget() const;

    /// Unregister so this shortcut can no longer fire (e.g. before deleteLater).
    void clear();

Q_SIGNALS:
    void activated();

private:
    Qt::MouseButton button_;
    Qt::ShortcutContext context_;
    bool registered_ = false;
};

/// Install the application event filter that dispatches MouseShortcut presses.
void installMouseShortcutFilter();

/// While set, extra mouse buttons are delivered here instead of MouseShortcuts.
void setMouseHotkeyCapture(QObject *owner,
                           std::function<void(Qt::MouseButton)> callback);

/// Clears capture only if @p owner is the current capture owner.
void clearMouseHotkeyCapture(QObject *owner);

}  // namespace chatterino
