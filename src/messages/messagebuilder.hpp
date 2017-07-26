#pragma once

#include "messages/message.hpp"

#include <ctime>
#include <QRegularExpression>

namespace chatterino {
namespace messages {

class MessageBuilder
{
public:
    MessageBuilder();

    SharedMessage build();

    void appendWord(const Word &word);
    void appendTimestamp();
    void appendTimestamp(std::time_t time);

    QString matchLink(const QString &string);
    QRegularExpression regex;

    QString originalMessage;

private:
    std::vector<Word> _words;
    std::chrono::time_point<std::chrono::system_clock> _parseTime;
};

}  // namespace messages
}  // namespace chatterino
