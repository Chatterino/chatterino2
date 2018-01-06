#include "messages/message.hpp"
#include "channel.hpp"
#include "emojis.hpp"
#include "messages/link.hpp"
#include "singletons/emotemanager.hpp"
#include "singletons/fontmanager.hpp"
#include "singletons/ircmanager.hpp"
#include "singletons/resourcemanager.hpp"
#include "singletons/thememanager.hpp"
#include "util/irchelpers.hpp"

#include <ctime>
#include <list>
#include <tuple>

typedef chatterino::widgets::ScrollbarHighlight SBHighlight;

namespace chatterino {
namespace messages {

bool Message::containsHighlightedPhrase() const
{
    return this->highlightTab;
}

void Message::setHighlight(bool value)
{
    this->highlightTab = value;
}

const QString &Message::getTimeoutUser() const
{
    return this->timeoutUser;
}

int Message::getTimeoutCount() const
{
    return this->timeoutCount;
}

const QString &Message::getContent() const
{
    if (this->content.isNull()) {
        this->updateContent();
    }

    return this->content;
}

const std::chrono::time_point<std::chrono::system_clock> &Message::getParseTime() const
{
    return this->parseTime;
}

std::vector<Word> &Message::getWords()
{
    return this->words;
}

Message::MessageFlags Message::getFlags() const
{
    return this->flags;
}

void Message::setFlags(MessageFlags _flags)
{
    this->flags = flags;
}

void Message::addFlags(MessageFlags _flags)
{
    this->flags = (MessageFlags)((MessageFlagsType)this->flags | (MessageFlagsType)_flags);
}

void Message::removeFlags(MessageFlags _flags)
{
    this->flags = (MessageFlags)((MessageFlagsType)this->flags & ~((MessageFlagsType)_flags));
}

bool Message::isDisabled() const
{
    return this->disabled;
}

void Message::setDisabled(bool value)
{
    this->disabled = value;
}

const QString &Message::getId() const
{
    return this->id;
}

bool Message::getCollapsedDefault() const
{
    return this->collapsedDefault;
}

void Message::setCollapsedDefault(bool value)
{
    this->collapsedDefault = value;
}

bool Message::getDisableCompactEmotes() const
{
    return this->disableCompactEmotes;
}

void Message::setDisableCompactEmotes(bool value)
{
    this->disableCompactEmotes = value;
}

void Message::updateContent() const
{
    QString _content("");

    bool first;

    for (const Word &word : this->words) {
        if (!first) {
            _content += "";
        }

        _content += word.getCopyText();
        first = false;
    }

    this->content = _content;
}

SBHighlight Message::getScrollBarHighlight() const
{
    if (this->getFlags() & Message::Highlighted) {
        return SBHighlight(SBHighlight::Highlight);
    }
    return SBHighlight();
}

namespace {

void AddCurrentTimestamp(Message *message)
{
    std::time_t t;
    time(&t);
    char timeStampBuffer[69];

    // Add word for timestamp with no seconds
    strftime(timeStampBuffer, 69, "%H:%M", localtime(&t));
    QString timestampNoSeconds(timeStampBuffer);
    message->getWords().push_back(Word(timestampNoSeconds, Word::TimestampNoSeconds,
                                       MessageColor(MessageColor::System),
                                       singletons::FontManager::Medium, QString(), QString()));

    // Add word for timestamp with seconds
    strftime(timeStampBuffer, 69, "%H:%M:%S", localtime(&t));
    QString timestampWithSeconds(timeStampBuffer);
    message->getWords().push_back(Word(timestampWithSeconds, Word::TimestampWithSeconds,
                                       MessageColor(MessageColor::System),
                                       singletons::FontManager::Medium, QString(), QString()));
}

}  // namespace

/// Static
Message *Message::createSystemMessage(const QString &text)
{
    Message *message = new Message;

    AddCurrentTimestamp(message);

    QStringList words = text.split(' ');

    for (QString word : words) {
        message->getWords().push_back(Word(word, Word::Flags::Default,
                                           MessageColor(MessageColor::Type::System),
                                           singletons::FontManager::Medium, word, QString()));
    }
    message->addFlags(Message::System);

    return message;
}

Message *Message::createTimeoutMessage(const QString &username, const QString &durationInSeconds,
                                       const QString &reason, bool multipleTimes)
{
    QString text;

    text.append(username);
    if (!durationInSeconds.isEmpty()) {
        text.append(" has been timed out");

        // TODO: Implement who timed the user out

        text.append(" for ");
        text.append(durationInSeconds);
        bool ok = true;
        int timeoutDuration = durationInSeconds.toInt(&ok);
        text.append(" second");
        if (ok && timeoutDuration > 1) {
            text.append("s");
        }
    } else {
        text.append(" has been permanently banned");
    }

    if (reason.length() > 0) {
        text.append(": \"");
        text.append(ParseTagString(reason));
        text.append("\"");
    }
    text.append(".");

    if (multipleTimes) {
        text.append(" (multiple times)");
    }

    Message *message = Message::createSystemMessage(text);
    message->addFlags(Message::Timeout);
    message->timeoutUser = username;
    return message;
}

}  // namespace messages
}  // namespace chatterino
