#include "MessageBuilder.hpp"
#include "singletons/EmoteManager.hpp"
#include "singletons/ResourceManager.hpp"
#include "singletons/ThemeManager.hpp"

#include <QDateTime>

namespace chatterino {

MessageBuilder::MessageBuilder()
    : message(new Message)
{
}

MessagePtr MessageBuilder::getMessage()
{
    return this->message;
}

void MessageBuilder::append(MessageElement *element)
{
    this->message->addElement(element);
}

void MessageBuilder::appendTimestamp()
{
    this->appendTimestamp(QTime::currentTime());
}

void MessageBuilder::setHighlight(bool value)
{
    if (value) {
        this->message->flags |= Message::Highlighted;
    } else {
        this->message->flags &= ~Message::Highlighted;
    }
}

void MessageBuilder::appendTimestamp(const QTime &time)
{
    this->append(new TimestampElement(time));
}

QString MessageBuilder::matchLink(const QString &string)
{
    static QRegularExpression linkRegex("[[:ascii:]]*\\.[a-zA-Z]+\\/?[[:ascii:]]*");
    static QRegularExpression httpRegex("\\bhttps?://");

    auto match = linkRegex.match(string);

    if (!match.hasMatch()) {
        return QString();
    }

    QString captured = match.captured();

    if (!captured.contains(httpRegex)) {
        captured.insert(0, "http://");
    }

    return captured;
}

}  // namespace chatterino
