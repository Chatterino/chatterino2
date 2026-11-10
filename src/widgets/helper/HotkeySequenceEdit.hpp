// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/hotkeys/HotkeySequence.hpp"

#include <QKeySequenceEdit>

class QFocusEvent;
class QKeyEvent;
class QLineEdit;

namespace chatterino {

/**
 * QKeySequenceEdit that can also record extra mouse buttons (Back/Forward, 6+).
 *
 * Left, Right, and Middle are ignored so they cannot steal clicking.
 */
class HotkeySequenceEdit : public QKeySequenceEdit
{
    Q_OBJECT

public:
    explicit HotkeySequenceEdit(QWidget *parent = nullptr);

    [[nodiscard]] HotkeySequence sequence() const;
    void setSequence(const HotkeySequence &sequence);

Q_SIGNALS:
    void sequenceChanged(const chatterino::HotkeySequence &sequence);

protected:
    /// Installed on the application while focused to record extra mouse buttons.
    bool eventFilter(QObject *watched, QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void recordMouseButton(Qt::MouseButton button);
    void applyMouseDisplay();
    void emitSequenceChanged();
    QLineEdit *lineEdit() const;

    Qt::MouseButton mouseButton_ = Qt::NoButton;
    bool updatingMouseDisplay_ = false;
};

}  // namespace chatterino
