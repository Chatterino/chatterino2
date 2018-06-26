#pragma once

#include <QHBoxLayout>
#include <QScrollArea>
#include <QTabWidget>
#include <QWidget>

#include <cassert>
#include <type_traits>

namespace chatterino {
namespace util {

template <class T>
class LayoutCreator
{
public:
    LayoutCreator(T *_item)
        : item(_item)
    {
    }

    T *operator->()
    {
        return this->item;
    }

    T &operator*()
    {
        return *this->item;
    }

    T *getElement()
    {
        return this->item;
    }

    template <typename T2>
    LayoutCreator<T2> append(T2 *_item)
    {
        this->_addItem(this->getOrCreateLayout(), _item);

        return LayoutCreator<T2>(_item);
    }

    template <typename T2, typename... Args>
    LayoutCreator<T2> emplace(Args &&... args)
    {
        T2 *t = new T2(std::forward<Args>(args)...);

        this->_addItem(this->getOrCreateLayout(), t);

        return LayoutCreator<T2>(t);
    }

    template <typename Q = T,
              typename std::enable_if<std::is_base_of<QScrollArea, Q>::value, int>::type = 0>
    LayoutCreator<QWidget> emplaceScrollAreaWidget()
    {
        QWidget *widget = new QWidget;
        this->item->setWidget(widget);
        return LayoutCreator<QWidget>(widget);
    }

    template <typename T2, typename Q = T,
              typename std::enable_if<std::is_base_of<QWidget, Q>::value, int>::type = 0,
              typename std::enable_if<std::is_base_of<QLayout, T2>::value, int>::type = 0>
    LayoutCreator<T2> setLayoutType()
    {
        T2 *layout = new T2;

        this->item->setLayout(layout);

        return LayoutCreator<T2>(layout);
    }

    LayoutCreator<T> assign(T **ptr)
    {
        *ptr = this->item;

        return *this;
    }

    template <typename Q = T,
              typename std::enable_if<std::is_base_of<QLayout, Q>::value, int>::type = 0>
    LayoutCreator<T> withoutMargin()
    {
        this->item->setContentsMargins(0, 0, 0, 0);

        return *this;
    }

    template <typename Q = T,
              typename std::enable_if<std::is_base_of<QWidget, Q>::value, int>::type = 0>
    LayoutCreator<T> hidden()
    {
        this->item->setVisible(false);

        return *this;
    }

    template <typename Q = T, typename T2,
              typename std::enable_if<std::is_same<QTabWidget, Q>::value, int>::type = 0>
    LayoutCreator<T2> appendTab(T2 *item, const QString &title)
    {
        static_assert(std::is_base_of<QLayout, T2>::value, "needs to be QLayout");

        QWidget *widget = new QWidget;
        widget->setLayout(item);

        this->item->addTab(widget, title);

        return LayoutCreator<T2>(item);
    }

private:
    T *item;

    template <typename T2,
              typename std::enable_if<std::is_base_of<QWidget, T2>::value, int>::type = 0>
    void _addItem(QLayout *layout, T2 *item)
    {
        layout->addWidget(item);
    }

    template <typename T2,
              typename std::enable_if<std::is_base_of<QLayout, T2>::value, int>::type = 0>
    void _addItem(QLayout *layout, T2 *item)
    {
        QWidget *widget = new QWidget();
        widget->setLayout(item);
        layout->addWidget(widget);
    }

    template <typename Q = T,
              typename std::enable_if<std::is_base_of<QLayout, Q>::value, int>::type = 0>
    QLayout *getOrCreateLayout()
    {
        return this->item;
    }

    template <typename Q = T,
              typename std::enable_if<std::is_base_of<QWidget, Q>::value, int>::type = 0>
    QLayout *getOrCreateLayout()
    {
        if (!this->item->layout()) {
            this->item->setLayout(new QHBoxLayout());
        }

        return this->item->layout();
    }
};

}  // namespace util
}  // namespace chatterino
