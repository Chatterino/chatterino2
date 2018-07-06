#pragma once

#include "messages/Message.hpp"

#include <QRegularExpression>

#include <ctime>

namespace chatterino {

struct MessageBuilder {
public:
    MessageBuilder();

    MessagePtr getMessage();

    void setHighlight(bool value);
    void append(MessageElement *element);
    void appendTimestamp();
    void appendTimestamp(const QTime &time);
    QString matchLink(const QString &string);

    template <typename T, typename... Args>
    T *emplace(Args &&... args)
    {
        static_assert(std::is_base_of<MessageElement, T>::value, "T must extend MessageElement");

        T *element = new T(std::forward<Args>(args)...);
        this->append(element);
        return element;
    }

protected:
    MessagePtr message_;
};

}  // namespace chatterino
