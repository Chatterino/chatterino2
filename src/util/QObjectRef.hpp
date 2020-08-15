#pragma once

#include <QObject>
#include <type_traits>

namespace chatterino {
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

    explicit QObjectRef(T *t)
    {
        static_assert(std::is_base_of_v<QObject, T>);

        this->set(t);
    }

    QObjectRef(const QObjectRef &other)
    {
        this->set(other.t_);
    }

    ~QObjectRef()
    {
        this->set(nullptr);
    }

    QObjectRef &operator=(T *t)
    {
        this->set(t);

        return *this;
    }

    operator bool()
    {
        return t_;
    }

    T *operator->()
    {
        return t_;
    }

    T *get()
    {
        return t_;
    }

private:
    void set(T *other)
    {
        // old
        if (this->conn_)
        {
            QObject::disconnect(this->conn_);
        }

        // new
        if (other)
        {
            this->conn_ = QObject::connect(
                other, &QObject::destroyed, qApp,
                [this](QObject *) { this->set(nullptr); },
                Qt::DirectConnection);
        }

        this->t_ = other;
    }

    std::atomic<T *> t_{};
    QMetaObject::Connection conn_;
};
}  // namespace chatterino
