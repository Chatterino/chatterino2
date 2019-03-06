#pragma once

#include <QLayout>
#include <QWidget>
#include <initializer_list>

class QBoxLayout;

namespace ab
{
    /// You may pass this to an element to ab::makeLayout to insert a stretch
    /// item.
    ///
    /// Check ab::addStretch to add support for another layout type.
    QObject* stretch();

    /// This function is used to add support for passing ab::stretch() as a
    /// parameter to ab::makeLayout().
    ///
    /// Create a `void addStretch(YourType *)` to add support.
    void addStretch(void*);
    void addStretch(QBoxLayout* box);

    /// Creates a widget of type T, and applies the function UnaryWith on it.
    ///
    /// This allows for a "declarative" style of defining layouts and widget
    /// children.
    template <typename T, typename UnaryWith>
    T* makeWidget(UnaryWith with)
    {
        auto t = new T;

        with(t);

        return t;
    }

    /// Creates a layout of type T, and adds all elements in widgets.
    ///
    /// @param widgets May contain ab::stretch() to add a stretch item.
    template <typename T>
    T* makeLayout(std::initializer_list<QObject*> widgets)
    {
        auto t = new T;

        for (auto&& widget : widgets)
        {
            if (widget == nullptr)
                continue;
            else if (widget == stretch())
                addStretch(t);
            else if (widget->isWidgetType())
                t->addWidget(static_cast<QWidget*>(widget));
            else if (auto layout = dynamic_cast<QLayout*>(widget))
                t->addLayout(layout);
            else
                assert(false);
        }

        t->setContentsMargins(0, 0, 0, 0);

        return t;
    }

    /// Creates a layout of type T, applies the function UnaryWith, and adds all
    /// elements in widgets.
    ///
    /// This allows for a "declarative" style of defining layouts and widget
    /// children.
    ///
    /// @param widgets May contain ab::stretch() to add a stretch item.
    template <typename T, typename UnaryWith>
    T* makeLayout(UnaryWith with, std::initializer_list<QObject*> widgets)
    {
        auto t = makeLayout<T>(widgets);

        with(t);

        return t;
    }
}  // namespace ab
