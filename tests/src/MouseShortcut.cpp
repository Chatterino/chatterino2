// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/hotkeys/MouseShortcut.hpp"

#include "Test.hpp"
#include "widgets/helper/HotkeySequenceEdit.hpp"

#include <QApplication>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QWidget>

using namespace chatterino;

namespace {

void press(QWidget *target, Qt::MouseButton button)
{
    QMouseEvent event(QEvent::MouseButtonPress, QPointF(1, 1), QPointF(1, 1),
                      button, button, Qt::NoModifier);
    QApplication::sendEvent(target, &event);
}

}  // namespace

TEST(MouseShortcut, ActivatesOnMatchingButton)
{
    QWidget parent;
    parent.show();
    MouseShortcut shortcut(Qt::BackButton, Qt::ApplicationShortcut, &parent);
    int activations = 0;
    QObject::connect(&shortcut, &MouseShortcut::activated, [&] {
        activations++;
    });

    press(&parent, Qt::ForwardButton);
    press(&parent, Qt::LeftButton);
    EXPECT_EQ(activations, 0);

    press(&parent, Qt::BackButton);
    EXPECT_EQ(activations, 1);
}

TEST(MouseShortcut, ClearedShortcutDoesNotActivate)
{
    QWidget parent;
    parent.show();
    MouseShortcut shortcut(Qt::BackButton, Qt::ApplicationShortcut, &parent);
    int activations = 0;
    QObject::connect(&shortcut, &MouseShortcut::activated, [&] {
        activations++;
    });

    shortcut.clear();
    press(&parent, Qt::BackButton);
    EXPECT_EQ(activations, 0);
}

TEST(MouseShortcut, HiddenParentDoesNotActivate)
{
    QWidget parent;
    MouseShortcut shortcut(Qt::BackButton, Qt::ApplicationShortcut, &parent);
    int activations = 0;
    QObject::connect(&shortcut, &MouseShortcut::activated, [&] {
        activations++;
    });

    press(&parent, Qt::BackButton);
    EXPECT_EQ(activations, 0);
}

TEST(MouseShortcut, FocusedHotkeySequenceEditCapturesFirst)
{
    QWidget parent;
    parent.show();
    MouseShortcut shortcut(Qt::BackButton, Qt::ApplicationShortcut, &parent);
    int activations = 0;
    QObject::connect(&shortcut, &MouseShortcut::activated, [&] {
        activations++;
    });

    HotkeySequenceEdit edit;
    QFocusEvent focusIn(QEvent::FocusIn, Qt::OtherFocusReason);
    QApplication::sendEvent(&edit, &focusIn);

    press(&parent, Qt::BackButton);
    EXPECT_EQ(activations, 0);
    EXPECT_EQ(edit.sequence(), HotkeySequence(Qt::BackButton));

    QFocusEvent focusOut(QEvent::FocusOut, Qt::OtherFocusReason);
    QApplication::sendEvent(&edit, &focusOut);

    press(&parent, Qt::BackButton);
    EXPECT_EQ(activations, 1);
    EXPECT_EQ(edit.sequence(), HotkeySequence(Qt::BackButton));
}
