// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/hotkeys/MouseShortcut.hpp"

#include "controllers/hotkeys/HotkeySequence.hpp"

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QPointer>
#include <QWidget>

#include <algorithm>
#include <vector>

namespace {

using namespace chatterino;

std::vector<MouseShortcut *> mouseShortcuts;
std::function<void(Qt::MouseButton)> mouseCapture;
QPointer<QObject> mouseCaptureOwner;
QPointer<QObject> installedMouseShortcutFilter;

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

void unregisterMouseShortcut(MouseShortcut *shortcut)
{
    auto it = std::ranges::find(mouseShortcuts, shortcut);
    if (it != mouseShortcuts.end())
    {
        mouseShortcuts.erase(it);
    }
}

class MouseShortcutEventFilter : public QObject
{
public:
    explicit MouseShortcutEventFilter(QObject *parent)
        : QObject(parent)
    {
    }

    ~MouseShortcutEventFilter() override
    {
        if (qApp != nullptr)
        {
            qApp->removeEventFilter(this);
        }
        installedMouseShortcutFilter.clear();
    }

    bool eventFilter(QObject * /*watched*/, QEvent *event) override
    {
        if (event->type() != QEvent::MouseButtonPress)
        {
            return false;
        }

        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        auto button = mouseEvent->button();
        if (!isExtraMouseButton(button))
        {
            return false;
        }

        if (mouseCapture)
        {
            mouseCapture(button);
            return true;
        }

        QWidget *focusWidget = QApplication::focusWidget();
        MouseShortcut *best = nullptr;
        int bestPriority = -1;
        for (auto *shortcut : mouseShortcuts)
        {
            if (shortcut->button() != button)
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
};

}  // namespace

namespace chatterino {

void installMouseShortcutFilter()
{
    if (installedMouseShortcutFilter || qApp == nullptr)
    {
        return;
    }

    auto *filter = new MouseShortcutEventFilter(qApp);
    installedMouseShortcutFilter = filter;
    qApp->installEventFilter(filter);
}

void setMouseHotkeyCapture(QObject *owner,
                           std::function<void(Qt::MouseButton)> callback)
{
    mouseCaptureOwner = owner;
    mouseCapture = std::move(callback);
}

void clearMouseHotkeyCapture(QObject *owner)
{
    if (mouseCaptureOwner != owner)
    {
        return;
    }

    mouseCaptureOwner.clear();
    mouseCapture = {};
}

MouseShortcut::MouseShortcut(Qt::MouseButton button,
                             Qt::ShortcutContext context, QWidget *parent)
    : QObject(parent)
    , button_(button)
    , context_(context)
{
    installMouseShortcutFilter();
    mouseShortcuts.push_back(this);
    this->registered_ = true;
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
    if (!this->registered_)
    {
        return;
    }

    unregisterMouseShortcut(this);
    this->registered_ = false;
}

}  // namespace chatterino
