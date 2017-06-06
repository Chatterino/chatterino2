#include "messagebuilder.h"
#include "colorscheme.h"
#include "emotemanager.h"
#include "resources.h"

namespace chatterino {
namespace messages {

MessageBuilder::MessageBuilder()
    : _words()
{
    _parseTime = std::chrono::system_clock::now();
}

SharedMessage MessageBuilder::build()
{
    return SharedMessage(new Message(this->originalMessage, _words));
}

void MessageBuilder::appendWord(const Word &word)
{
    _words.push_back(word);
}

void MessageBuilder::appendTimestamp()
{
    time_t t;
    time(&t);
    appendTimestamp(t);
}

void MessageBuilder::appendTimestamp(time_t time)
{
    char timeStampBuffer[69];

    strftime(timeStampBuffer, 69, "%H:%M", localtime(&time));
    QString timestamp = QString(timeStampBuffer);

    strftime(timeStampBuffer, 69, "%H:%M:%S", localtime(&time));
    QString timestampWithSeconds = QString(timeStampBuffer);

    appendWord(Word(timestamp, Word::TimestampNoSeconds,
                    ColorScheme::getInstance().SystemMessageColor, QString(), QString()));
    appendWord(Word(timestampWithSeconds, Word::TimestampWithSeconds,
                    ColorScheme::getInstance().SystemMessageColor, QString(), QString()));
}

QString MessageBuilder::matchLink(const QString &string)
{
    // TODO: Implement this xD
    return QString();
}
}
}
