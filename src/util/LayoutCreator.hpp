#pragma once

#include <QHBoxLayout>
#include <QScrollArea>
#include <QTabWidget>
#include <QWidget>

#include <cassert>
#include <concepts>

namespace chatterino {

template <class T>
    requires std::derived_from<T, QWidget> || std::derived_from<T, QLayout>
class LayoutCreator
{
public:
    LayoutCreator(T *_item)
        : item_(_item)
    {
    }

    T *operator->()
    {
        return this->item_;
    }

    T &operator*()
    {
        return *this->item_;
    }

    T *getElement()
    {
        return this->item_;
    }

    template <typename U>
    LayoutCreator<U> append(U *item)
    {
        addItem(this->getOrCreateLayout(), item);

        return LayoutCreator<U>(item);
    }

    template <typename U>
    LayoutCreator<U> emplace(auto &&...args)
    {
        auto *t = new U(std::forward<decltype(args)>(args)...);

        addItem(this->getOrCreateLayout(), t);

        return LayoutCreator<U>(t);
    }

    LayoutCreator<QWidget> emplaceScrollAreaWidget()
        requires std::derived_from<T, QScrollArea>
    {
        auto *widget = new QWidget;
        this->item_->setWidget(widget);
        return {widget};
    }

    template <std::derived_from<QLayout> U>
    LayoutCreator<U> setLayoutType()
        requires std::derived_from<T, QWidget>
    {
        U *layout = new U;

        this->item_->setLayout(layout);

        return LayoutCreator<U>(layout);
    }

    LayoutCreator<T> assign(T **ptr)
    {
        *ptr = this->item_;

        return *this;
    }

    LayoutCreator<T> withoutMargin()
    {
        this->item_->setContentsMargins(0, 0, 0, 0);

        return *this;
    }

    LayoutCreator<T> withoutSpacing()
        requires std::derived_from<T, QLayout>
    {
        this->item_->setSpacing(0);

        return *this;
    }

    LayoutCreator<T> hidden()
        requires std::derived_from<T, QWidget>
    {
        this->item_->setVisible(false);

        return *this;
    }

    template <std::derived_from<QLayout> U>
    LayoutCreator<U> appendTab(U *item, const QString &title)
        requires std::derived_from<T, QWidget>
    {
        auto *widget = new QWidget;
        widget->setLayout(item);

        this->item_->addTab(widget, title);

        return LayoutCreator<U>(item);
    }

    template <typename Slot, typename Func>
    LayoutCreator<T> connect(Slot slot, QObject *receiver, Func func)
    {
        QObject::connect(this->getElement(), slot, receiver, func);
        return *this;
    }

    template <typename Func>
    LayoutCreator<T> onClick(QObject *receiver, Func func)
    {
        QObject::connect(this->getElement(), &T::clicked, receiver, func);
        return *this;
    }

private:
    T *item_;

    static void addItem(QLayout *layout, QWidget *item)
    {
        layout->addWidget(item);
    }

    static void addItem(QLayout *layout, QLayout *item)
    {
        auto *widget = new QWidget();
        widget->setLayout(item);
        layout->addWidget(widget);
    }

    QLayout *getOrCreateLayout()
        requires std::derived_from<T, QLayout>
    {
        return this->item_;
    }

    QLayout *getOrCreateLayout()
        requires std::derived_from<T, QWidget>
    {
        if (!this->item_->layout())
        {
            this->item_->setLayout(new QHBoxLayout());
        }

        return this->item_->layout();
    }
};

template <typename T>
LayoutCreator<T> makeDialog(auto &&...args)
{
    T *t = new T(std::forward<decltype(args)>(args)...);
    t->setAttribute(Qt::WA_DeleteOnClose);
    return LayoutCreator<T>(t);
}

}  // namespace chatterino
