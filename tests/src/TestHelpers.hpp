#pragma once

#include <QString>
#include <QStringView>

#include <mutex>
#include <ostream>

template <typename T>
class ReceivedMessage
{
    mutable std::mutex mutex;

    bool isSet{false};
    T t;

public:
    ReceivedMessage() = default;

    explicit operator bool() const
    {
        std::unique_lock lock(this->mutex);

        return this->isSet;
    }

    ReceivedMessage &operator=(const T &newT)
    {
        std::unique_lock lock(this->mutex);

        this->isSet = true;
        this->t = newT;

        return *this;
    }

    bool operator==(const T &otherT) const
    {
        std::unique_lock lock(this->mutex);

        return this->t == otherT;
    }

    const T *operator->() const
    {
        return &this->t;
    }
};

inline std::ostream &operator<<(std::ostream &os, const QStringView &str)
{
    os << qUtf8Printable(str.toString());
    return os;
}

inline std::ostream &operator<<(std::ostream &os, const QByteArray &bytes)
{
    os << bytes.toStdString();
    return os;
}

inline std::ostream &operator<<(std::ostream &os, const QString &str)
{
    os << qUtf8Printable(str);
    return os;
}
