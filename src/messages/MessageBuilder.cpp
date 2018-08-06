#include "MessageBuilder.hpp"

#include "common/LinkParser.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Theme.hpp"

#include <QDateTime>

namespace chatterino {

MessageBuilder::MessageBuilder()
    : message_(new Message)
{
}

MessagePtr MessageBuilder::getMessage()
{
    return this->message_;
}

void MessageBuilder::append(MessageElement *element)
{
    this->message_->addElement(element);
}

void MessageBuilder::appendTimestamp()
{
    this->appendTimestamp(QTime::currentTime());
}

void MessageBuilder::setHighlight(bool value)
{
    if (value) {
        this->message_->flags |= Message::Highlighted;
    } else {
        this->message_->flags &= ~Message::Highlighted;
    }
}

void MessageBuilder::appendTimestamp(const QTime &time)
{
    this->append(new TimestampElement(time));
}

QString MessageBuilder::matchLink(const QString &string)
{
    LinkParser linkParser(string);

    static QRegularExpression httpRegex(
        "\\bhttps?://", QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression ftpRegex(
        "\\bftps?://", QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression spotifyRegex(
        "\\bspotify:", QRegularExpression::CaseInsensitiveOption);

    if (!linkParser.hasMatch()) {
        return QString();
    }

    QString captured = linkParser.getCaptured();

    if (!captured.contains(httpRegex) && !captured.contains(ftpRegex) &&
        !captured.contains(spotifyRegex)) {
        captured.insert(0, "http://");
    }

    return captured;
}

}  // namespace chatterino
