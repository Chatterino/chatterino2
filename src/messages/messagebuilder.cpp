#include "messagebuilder.hpp"
#include "singletons/thememanager.hpp"
#include "singletons/emotemanager.hpp"
#include "singletons/resourcemanager.hpp"

#include <QDateTime>

namespace chatterino {
namespace messages {

MessageBuilder::MessageBuilder()
    : message(new Message)
{
    _parseTime = std::chrono::system_clock::now();
}

SharedMessage MessageBuilder::getMessage()
{
    return this->message;
}

void MessageBuilder::appendWord(const Word &&word)
{
    this->message->getWords().push_back(word);
}

void MessageBuilder::appendTimestamp()
{
    QDateTime t = QDateTime::currentDateTime();
    this->appendTimestamp(t);
}

void MessageBuilder::setHighlight(bool value)
{
    this->message->setHighlight(value);
}

void MessageBuilder::appendTimestamp(QDateTime &time)
{
    // Add word for timestamp with no seconds
    QString timestampNoSeconds(time.toString("hh:mm"));
    this->appendWord(Word(timestampNoSeconds, Word::TimestampNoSeconds,
                          MessageColor(MessageColor::System), singletons::FontManager::Medium, QString(),
                          QString()));

    // Add word for timestamp with seconds
    QString timestampWithSeconds(time.toString("hh:mm:ss"));
    this->appendWord(Word(timestampWithSeconds, Word::TimestampWithSeconds,
                          MessageColor(MessageColor::System), singletons::FontManager::Medium, QString(),
                          QString()));
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

}  // namespace messages
}  // namespace chatterino
