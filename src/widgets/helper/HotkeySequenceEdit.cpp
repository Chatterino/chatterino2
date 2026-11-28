// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/helper/HotkeySequenceEdit.hpp"

#include "controllers/hotkeys/HotkeyHelpers.hpp"

#include <QApplication>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QSignalBlocker>

namespace chatterino {

HotkeySequenceEdit::HotkeySequenceEdit(QWidget *parent)
    : QKeySequenceEdit(parent)
{
    if (auto *edit = this->lineEdit())
    {
        QObject::connect(
            edit, &QLineEdit::textChanged, this, [this](const QString &text) {
                if (this->updatingMouseDisplay_)
                {
                    return;
                }
                if (text.isEmpty() && this->mouseButton_ != Qt::NoButton)
                {
                    this->mouseButton_ = Qt::NoButton;
                    this->emitSequenceChanged();
                }
            });
    }

    QObject::connect(
        this, &QKeySequenceEdit::keySequenceChanged, this,
        [this](const QKeySequence &seq) {
            if (this->updatingMouseDisplay_)
            {
                return;
            }

            if (seq.isEmpty())
            {
                // QKeySequenceEdit::focusOutEvent calls finishEditing(), which
                // emits an empty key sequence. Keep a recorded mouse button.
                if (this->mouseButton_ != Qt::NoButton)
                {
                    return;
                }
                this->emitSequenceChanged();
                return;
            }

            this->mouseButton_ = Qt::NoButton;
            auto normalized = normalizeKeySequence(seq);
            if (normalized != seq)
            {
                QSignalBlocker blocker(this);
                this->setKeySequence(normalized);
            }
            this->emitSequenceChanged();
        });
}

HotkeySequence HotkeySequenceEdit::sequence() const
{
    if (this->mouseButton_ != Qt::NoButton)
    {
        return HotkeySequence(this->mouseButton_);
    }
    return {this->keySequence()};
}

void HotkeySequenceEdit::setSequence(const HotkeySequence &sequence)
{
    if (sequence.isMouse())
    {
        this->mouseButton_ = sequence.mouseButton();
        this->applyMouseDisplay();
        this->emitSequenceChanged();
        return;
    }

    this->mouseButton_ = Qt::NoButton;
    this->setKeySequence(sequence.keySequence());
    if (sequence.isEmpty())
    {
        this->emitSequenceChanged();
    }
}

void HotkeySequenceEdit::recordMouseButton(Qt::MouseButton button)
{
    this->mouseButton_ = button;
    this->applyMouseDisplay();
    this->emitSequenceChanged();
}

void HotkeySequenceEdit::focusInEvent(QFocusEvent *event)
{
    QKeySequenceEdit::focusInEvent(event);
    // Installed last so it runs before the MouseShortcut filter.
    qApp->installEventFilter(this);
    if (this->mouseButton_ != Qt::NoButton)
    {
        this->applyMouseDisplay();
    }
}

void HotkeySequenceEdit::focusOutEvent(QFocusEvent *event)
{
    auto savedMouse = this->mouseButton_;
    // Qt's focusOutEvent calls finishEditing(), which emits keySequenceChanged.
    // Swallow that only when restoring a mouse binding; keyboard chords must
    // notify listeners of the finalized sequence.
    this->updatingMouseDisplay_ = savedMouse != Qt::NoButton;
    QKeySequenceEdit::focusOutEvent(event);
    this->updatingMouseDisplay_ = false;
    qApp->removeEventFilter(this);

    if (savedMouse != Qt::NoButton)
    {
        this->mouseButton_ = savedMouse;
        this->applyMouseDisplay();
    }
}

bool HotkeySequenceEdit::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        auto *mouseEvent = dynamic_cast<QMouseEvent *>(event);
        if (mouseEvent != nullptr && isExtraMouseButton(mouseEvent->button()))
        {
            this->recordMouseButton(mouseEvent->button());
            return true;
        }
    }

    return QKeySequenceEdit::eventFilter(watched, event);
}

void HotkeySequenceEdit::keyPressEvent(QKeyEvent *event)
{
    const auto key = event->key();
    const bool isModifier = key == Qt::Key_Control || key == Qt::Key_Shift ||
                            key == Qt::Key_Meta || key == Qt::Key_Alt ||
                            key == Qt::Key_unknown;
    if (!isModifier && this->mouseButton_ != Qt::NoButton)
    {
        this->mouseButton_ = Qt::NoButton;
    }
    QKeySequenceEdit::keyPressEvent(event);
}

void HotkeySequenceEdit::applyMouseDisplay()
{
    this->updatingMouseDisplay_ = true;
    if (!this->keySequence().isEmpty())
    {
        this->setKeySequence({});
    }
    if (auto *edit = this->lineEdit())
    {
        edit->setText(HotkeySequence(this->mouseButton_).toString());
    }
    this->updatingMouseDisplay_ = false;
}

void HotkeySequenceEdit::emitSequenceChanged()
{
    Q_EMIT this->sequenceChanged(this->sequence());
}

QLineEdit *HotkeySequenceEdit::lineEdit() const
{
    return this->findChild<QLineEdit *>();
}

}  // namespace chatterino
