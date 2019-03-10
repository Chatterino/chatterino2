#pragma once

#include <QHBoxLayout>
#include <QScrollArea>
#include <QTabWidget>
#include <QWidget>

#include <cassert>
#include <type_traits>

namespace chatterino
{
    template <class T>
    class [[deprecated("use ab/MakeWidget instead")]] LayoutCreator {
    public:
        LayoutCreator(T * _item)
            : item_(_item)
        {
        }

        T* operator->()
        {
            return this->item_;
        }

        T& operator*()
        {
            return *this->item_;
        }

        T* getElement()
        {
            return this->item_;
        }

        template <typename T2>
        LayoutCreator<T2> append(T2 * _item)
        {
            this->addItem(this->getOrCreateLayout(), _item);

            return LayoutCreator<T2>(_item);
        }

        template <typename T2, typename... Args>
        LayoutCreator<T2> emplace(Args && ... args)
        {
            T2* t = new T2(std::forward<Args>(args)...);

            this->addItem(this->getOrCreateLayout(), t);

            return LayoutCreator<T2>(t);
        }

        template <typename Q = T,
            typename std::enable_if<std::is_base_of<QScrollArea, Q>::value,
                int>::type = 0>
        LayoutCreator<QWidget> emplaceScrollAreaWidget()
        {
            QWidget* widget = new QWidget;
            this->item_->setWidget(widget);
            return LayoutCreator<QWidget>(widget);
        }

        template <typename T2, typename Q = T,
            typename std::enable_if<std::is_base_of<QWidget, Q>::value,
                int>::type = 0,
            typename std::enable_if<std::is_base_of<QLayout, T2>::value,
                int>::type = 0>
        LayoutCreator<T2> setLayoutType()
        {
            T2* layout = new T2;

            this->item_->setLayout(layout);

            return LayoutCreator<T2>(layout);
        }

        LayoutCreator<T> assign(T * *ptr)
        {
            *ptr = this->item_;

            return *this;
        }

        template <typename Q = T,
            typename std::enable_if<std::is_base_of<QLayout, Q>::value,
                int>::type = 0>
        LayoutCreator<T> withoutMargin()
        {
            this->item_->setContentsMargins(0, 0, 0, 0);

            return *this;
        }

        LayoutCreator<T> withoutSpacing()
        {
            this->item_->setSpacing(0);

            return *this;
        }

        template <typename Q = T,
            typename std::enable_if<std::is_base_of<QWidget, Q>::value,
                int>::type = 0>
        LayoutCreator<T> hidden()
        {
            this->item_->setVisible(false);

            return *this;
        }

        template <typename Q = T, typename T2,
            typename std::enable_if<std::is_same<QTabWidget, Q>::value,
                int>::type = 0>
        LayoutCreator<T2> appendTab(T2 * item, const QString& title)
        {
            static_assert(
                std::is_base_of<QLayout, T2>::value, "needs to be QLayout");

            QWidget* widget = new QWidget;
            widget->setLayout(item);

            this->item_->addTab(widget, title);

            return LayoutCreator<T2>(item);
        }

    private:
        T* item_;

        template <typename T2,
            typename std::enable_if<std::is_base_of<QWidget, T2>::value,
                int>::type = 0>
        void addItem(QLayout * layout, T2 * item)
        {
            layout->addWidget(item);
        }

        template <typename T2,
            typename std::enable_if<std::is_base_of<QLayout, T2>::value,
                int>::type = 0>
        void addItem(QLayout * layout, T2 * item)
        {
            QWidget* widget = new QWidget();
            widget->setLayout(item);
            layout->addWidget(widget);
        }

        template <typename Q = T,
            typename std::enable_if<std::is_base_of<QLayout, Q>::value,
                int>::type = 0>
        QLayout* getOrCreateLayout()
        {
            return this->item_;
        }

        template <typename Q = T,
            typename std::enable_if<std::is_base_of<QWidget, Q>::value,
                int>::type = 0>
        QLayout* getOrCreateLayout()
        {
            if (!this->item_->layout())
            {
                this->item_->setLayout(new QHBoxLayout());
            }

            return this->item_->layout();
        }
    };

}  // namespace chatterino
