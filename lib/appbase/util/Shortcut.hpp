#pragma once

#include <QShortcut>
#include <QWidget>

namespace AB_NAMESPACE {

template <typename WidgetType, typename Func>
inline void createShortcut(WidgetType *w, const char *key, Func func)
{
    auto s = new QShortcut(QKeySequence(key), w);
    s->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(s, &QShortcut::activated, w, func);
}

template <typename WidgetType, typename Func>
inline void createWindowShortcut(WidgetType *w, const char *key, Func func)
{
    auto s = new QShortcut(QKeySequence(key), w);
    s->setContext(Qt::WindowShortcut);
    QObject::connect(s, &QShortcut::activated, w, func);
}

}  // namespace AB_NAMESPACE
