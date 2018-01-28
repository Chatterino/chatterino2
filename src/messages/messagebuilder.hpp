#pragma once

#include "messages/message.hpp"

#include <QRegularExpression>

#include <ctime>

namespace chatterino {
namespace messages {

struct MessageBuilder
{
public:
    MessageBuilder();

    MessagePtr getMessage();

    void setHighlight(bool value);
    void appendElement(MessageElement *element);

    //    typename std::enable_if<std::is_base_of<MessageElement, T>::value, T>::type

    template <class T, class... Args>
    T *emplace(Args &&... args)
    {
        static_assert(std::is_base_of<MessageElement, T>::value, "T must extend MessageElement");

        T *element = new T(std::forward<Args>(args)...);
        this->appendElement(element);
        return element;
    }

    void appendTimestamp();
    void appendTimestamp(const QTime &time);

    QString matchLink(const QString &string);

    QString originalMessage;

protected:
    MessagePtr message;
};

}  // namespace messages
}  // namespace chatterino
