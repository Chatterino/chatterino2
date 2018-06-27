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
    QFile tldFile(":/tlds.txt");
    tldFile.open(QFile::ReadOnly);
    QTextStream t1(&tldFile);
    t1.setCodec("UTF-8");
    QString tldData = t1.readAll();
    tldData.replace("\n", "|");
    const QString urlRegExp = "^"
    // protocol identifier
    "(?:(?:https?|ftps?)://)?"
    // user:pass authentication
    "(?:\\S+(?::\\S*)?@)?"
    "(?:"
    // IP address dotted notation octets
    // excludes loopback network 0.0.0.0
    // excludes reserved space >= 224.0.0.0
    // excludes network & broacast addresses
    // (first & last IP address of each class)
    "(?:[1-9]\\d?|1\\d\\d|2[01]\\d|22[0-3])"
    "(?:\\.(?:1?\\d{1,2}|2[0-4]\\d|25[0-5])){2}"
    "(?:\\.(?:[1-9]\\d?|1\\d\\d|2[0-4]\\d|25[0-4]))"
    "|"
    // host name
    "(?:(?:[_a-z\\x{00a1}-\\x{ffff}0-9]-*)*[a-z\\x{00a1}-\\x{ffff}0-9]+)"
    // domain name
    "(?:\\.(?:[a-z\\x{00a1}-\\x{ffff}0-9]-*)*[a-z\\x{00a1}-\\x{ffff}0-9]+)*"
    // TLD identifier
    //"(?:\\.(?:[a-z\\x{00a1}-\\x{ffff}]{2,}))"
    "(?:[\\.](?:" + tldData + "))"
    "\\.?"
    ")"
    // port number
    "(?::\\d{2,5})?"
    // resource path
    "(?:[/?#]\\S*)?"
    "$";
    static QRegularExpression linkRegex(urlRegExp, QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression httpRegex("\\bhttps?://");
    static QRegularExpression ftpRegex("\\bftps?://");
    auto match = linkRegex.match(string);

    if (!match.hasMatch()) {
        return QString();
    }

    QString captured = match.captured();

    if (!captured.contains(httpRegex)) {
        if (!captured.contains(ftpRegex)) {
            captured.insert(0, "http://");
        }
    }

    return captured;
}

}  // namespace chatterino
