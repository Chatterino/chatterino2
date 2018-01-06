#pragma once

#include <QShortcut>
#include <QWidget>

namespace chatterino {
namespace widgets {

template <typename WidgetType, typename Func>
inline void CreateShortcut(WidgetType *w, const char *key, Func func)
{
    auto s = new QShortcut(QKeySequence(key), w);
    s->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(s, &QShortcut::activated, w, func);
}

template <typename WidgetType, typename Func>
inline void CreateWindowShortcut(WidgetType *w, const char *key, Func func)
{
    auto s = new QShortcut(QKeySequence(key), w);
    s->setContext(Qt::WindowShortcut);
    QObject::connect(s, &QShortcut::activated, w, func);
}

}  // namespace widgets
}  // namespace chatterino
