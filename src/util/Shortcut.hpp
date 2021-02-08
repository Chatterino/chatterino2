#pragma once

#include <QShortcut>
#include <QWidget>

namespace chatterino {

template <typename WidgetType, typename Func>
[[deprecated("Use the actions/hotkeys system through HotkeyController")]]  //
inline void
    createShortcut(WidgetType *w, const char *key, Func func)
{
    auto s = new QShortcut(QKeySequence(key), w);
    s->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(s, &QShortcut::activated, w, func);
}

template <typename WidgetType, typename Func>
[[deprecated("Use the actions/hotkeys system through HotkeyController")]]  //
inline void
    createWindowShortcut(WidgetType *w, const char *key, Func func)
{
    auto s = new QShortcut(QKeySequence(key), w);
    s->setContext(Qt::WindowShortcut);
    QObject::connect(s, &QShortcut::activated, w, func);
}

}  // namespace chatterino
