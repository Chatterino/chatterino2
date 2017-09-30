#include "messagebuilder.hpp"
#include "colorscheme.hpp"
#include "emotemanager.hpp"
#include "resources.hpp"

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
    std::time_t t;
    time(&t);
    this->appendTimestamp(t);
}

void MessageBuilder::setHighlight(bool value)
{
    this->message->setHighlight(value);
}

void MessageBuilder::appendTimestamp(time_t time)
{
    char timeStampBuffer[69];

    // Add word for timestamp with no seconds
    strftime(timeStampBuffer, 69, "%H:%M", localtime(&time));
    QString timestampNoSeconds(timeStampBuffer);
    this->appendWord(Word(timestampNoSeconds, Word::TimestampNoSeconds,
                          MessageColor(MessageColor::System), QString()));

    // Add word for timestamp with seconds
    strftime(timeStampBuffer, 69, "%H:%M:%S", localtime(&time));
    QString timestampWithSeconds(timeStampBuffer);
    this->appendWord(Word(timestampWithSeconds, Word::TimestampWithSeconds,
                          MessageColor(MessageColor::System), QString()));
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
