#include "messagebuilder.hpp"
#include "colorscheme.hpp"
#include "emotemanager.hpp"
#include "resources.hpp"

namespace chatterino {
namespace messages {

MessageBuilder::MessageBuilder()
    : _words()
{
    _parseTime = std::chrono::system_clock::now();
    regex.setPattern("(\\bhttps?:\/\/)?[-A-Z0-9+&@#\/%?=~_|!:,.;]*[A-Z0-9+&@#\/%=~_|]");
    regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
}

SharedMessage MessageBuilder::build()
{
    return SharedMessage(new Message(this->originalMessage, _words,highlight));
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

void MessageBuilder::setHighlight(const bool &value){
    highlight = value;
}

void MessageBuilder::appendTimestamp(time_t time)
{
    char timeStampBuffer[69];

    // TODO(pajlada): Fix this
    QColor systemMessageColor(140, 127, 127);
    // QColor &systemMessageColor = ColorScheme::getInstance().SystemMessageColor;

    // Add word for timestamp with no seconds
    strftime(timeStampBuffer, 69, "%H:%M", localtime(&time));
    QString timestampNoSeconds(timeStampBuffer);
    appendWord(Word(timestampNoSeconds, Word::TimestampNoSeconds, systemMessageColor, QString(),
                    QString()));

    // Add word for timestamp with seconds
    strftime(timeStampBuffer, 69, "%H:%M:%S", localtime(&time));
    QString timestampWithSeconds(timeStampBuffer);
    appendWord(Word(timestampWithSeconds, Word::TimestampWithSeconds, systemMessageColor, QString(),
                    QString()));
}

QString MessageBuilder::matchLink(const QString &string)
{
    QString match = regex.match(string,0,QRegularExpression::PartialPreferCompleteMatch,QRegularExpression::NoMatchOption).captured();
    if(!match.contains(QRegularExpression("\\bhttps?:\/\/"))){
        match.insert(0,"https://");
    }
    return match;
}

}  // namespace messages
}  // namespace chatterino
