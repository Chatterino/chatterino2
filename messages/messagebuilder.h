#ifndef MESSAGEBUILDER_H
#define MESSAGEBUILDER_H

#include "messages/message.h"

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

private:
    std::vector<Word> _words;
    std::chrono::time_point<std::chrono::system_clock> _parseTime;
};
}
}

#endif  // MESSAGEBUILDER_H
