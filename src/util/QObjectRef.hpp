#pragma once

#include <QObject>
#include <type_traits>

namespace chatterino
{
    /// Holds a pointer to a QObject and resets it to nullptr if the QObject
    /// gets destroyed.
    template <typename T>
    class QObjectRef
    {
    public:
        QObjectRef()
        {
            static_assert(std::is_base_of_v<QObject, T>);
        }

        explicit QObjectRef(T* t)
        {
            static_assert(std::is_base_of_v<QObject, T>);

            this->swap(t);
        }

        ~QObjectRef()
        {
            this->swap(nullptr);
        }

        QObjectRef& operator=(T* t)
        {
            this->swap(t);
            return *this;
        }

        operator bool()
        {
            return t_;
        }

        T* operator->()
        {
            return t_;
        }

        T* get()
        {
            return t_;
        }

        QObject* swap(T* other)
        {
            // old
            if (this->conn_)
            {
                QObject::disconnect(this->conn_);
            }

            // new
            if (other)
            {
                QObject::connect(other, &QObject::destroyed,
                    [this]() { this->swap(nullptr); });
            }

            return std::exchange(this->t_, other);
        }

    private:
        T* t_{};
        QMetaObject::Connection conn_;
    };
}  // namespace chatterino
