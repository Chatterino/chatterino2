#include "MessageBuilder.hpp"

#include "common/LinkParser.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Theme.hpp"

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
    LinkParser linkParser(string);

    static QRegularExpression httpRegex("\\bhttps?://", QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression ftpRegex("\\bftps?://", QRegularExpression::CaseInsensitiveOption);

    if (!linkParser.hasMatch()) {
        return QString();
    }

    QString captured = linkParser.getCaptured();

    if (!captured.contains(httpRegex)) {
        if (!captured.contains(ftpRegex)) {
            captured.insert(0, "http://");
        }
    }

    return captured;
}

}  // namespace chatterino
