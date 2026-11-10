// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/hotkeys/MouseShortcut.hpp"

#include "controllers/hotkeys/HotkeySequence.hpp"
#include "util/FunctionEventFilter.hpp"

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QPointer>
#include <QWidget>

#include <vector>

namespace {

using namespace chatterino;

std::vector<MouseShortcut *> &mouseShortcuts()
{
    static std::vector<MouseShortcut *> shortcuts;
    return shortcuts;
}

int contextPriority(Qt::ShortcutContext context)
{
    switch (context)
    {
        case Qt::WidgetShortcut:
            return 3;
        case Qt::WidgetWithChildrenShortcut:
            return 2;
        case Qt::WindowShortcut:
            return 1;
        case Qt::ApplicationShortcut:
            return 0;
        default:
            return -1;
    }
}

bool isShortcutActive(const MouseShortcut *shortcut, QWidget *focusWidget)
{
    auto *parent = shortcut->parentWidget();
    if (parent == nullptr || !parent->isEnabled() || !parent->isVisible())
    {
        return false;
    }

    switch (shortcut->context())
    {
        case Qt::WindowShortcut: {
            auto *window = parent->window();
            return window != nullptr && window->isActiveWindow();
        }
        case Qt::WidgetWithChildrenShortcut:
            return focusWidget != nullptr &&
                   (parent == focusWidget || parent->isAncestorOf(focusWidget));
        case Qt::WidgetShortcut:
            return focusWidget == parent;
        case Qt::ApplicationShortcut:
            return true;
        default:
            return false;
    }
}

bool dispatchMouseShortcut(QObject * /*watched*/, QEvent *event)
{
    if (event->type() != QEvent::MouseButtonPress)
    {
        return false;
    }

    auto *mouseEvent = dynamic_cast<QMouseEvent *>(event);
    if (mouseEvent == nullptr || !isExtraMouseButton(mouseEvent->button()))
    {
        return false;
    }

    QWidget *focusWidget = QApplication::focusWidget();
    MouseShortcut *best = nullptr;
    int bestPriority = -1;
    for (auto *shortcut : mouseShortcuts())
    {
        if (shortcut->button() != mouseEvent->button())
        {
            continue;
        }
        if (!isShortcutActive(shortcut, focusWidget))
        {
            continue;
        }
        int priority = contextPriority(shortcut->context());
        if (priority > bestPriority)
        {
            bestPriority = priority;
            best = shortcut;
        }
    }

    if (best != nullptr)
    {
        Q_EMIT best->activated();
        return true;
    }

    return false;
}

}  // namespace

namespace chatterino {

void installMouseShortcutFilter()
{
    static QPointer<QObject> filter;
    if (filter || qApp == nullptr)
    {
        return;
    }

    filter = new FunctionEventFilter(qApp, dispatchMouseShortcut);
    qApp->installEventFilter(filter);
}

MouseShortcut::MouseShortcut(Qt::MouseButton button,
                             Qt::ShortcutContext context, QWidget *parent)
    : QObject(parent)
    , button_(button)
    , context_(context)
{
    installMouseShortcutFilter();
    mouseShortcuts().push_back(this);
}

MouseShortcut::~MouseShortcut()
{
    this->clear();
}

Qt::MouseButton MouseShortcut::button() const
{
    return this->button_;
}

Qt::ShortcutContext MouseShortcut::context() const
{
    return this->context_;
}

QWidget *MouseShortcut::parentWidget() const
{
    return qobject_cast<QWidget *>(this->parent());
}

void MouseShortcut::clear()
{
    std::erase(mouseShortcuts(), this);
}

}  // namespace chatterino
