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
    ~HotkeySequenceEdit() override;

    [[nodiscard]] HotkeySequence sequence() const;
    void setSequence(const HotkeySequence &sequence);

Q_SIGNALS:
    void sequenceChanged(const HotkeySequence &sequence);

protected:
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void recordMouseButton(Qt::MouseButton button);
    void applyMouseDisplay();
    void startMouseCapture();
    void stopMouseCapture();
    void emitSequenceChanged();
    QLineEdit *lineEdit() const;

    Qt::MouseButton mouseButton_ = Qt::NoButton;
    bool updatingMouseDisplay_ = false;
};

}  // namespace chatterino
