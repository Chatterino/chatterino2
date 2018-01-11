#include "messages/message.hpp"
#include "messageelement.hpp"
#include "util/irchelpers.hpp"

typedef chatterino::widgets::ScrollbarHighlight SBHighlight;

namespace chatterino {
namespace messages {

// elements
void Message::addElement(MessageElement *element)
{
    this->elements.push_back(std::unique_ptr<MessageElement>(element));
}

const std::vector<std::unique_ptr<MessageElement>> &Message::getElements() const
{
    return this->elements;
}

// Flags
Message::MessageFlags Message::getFlags() const
{
    return this->flags;
}

bool Message::hasFlags(MessageFlags _flags) const
{
    return this->flags & _flags;
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

// Parse Time
const QTime &Message::getParseTime() const
{
    return this->parseTime;
}

// Id
const QString &Message::getId() const
{
    return this->id;
}

void Message::setId(const QString &_id)
{
    this->id = _id;
}

// Search
const QString &Message::getSearchText() const
{
    // fourtf: asdf
    //    if (this->searchText.isNull()) {
    //        QString _content("");

    //        bool first;

    //        for (const MessageElement &word : this->words) {
    //            if (!first) {
    //                _content += "";
    //            }

    //            _content += word.getCopyText();
    //            first = false;
    //        }

    //        this->searchText = _content;
    //    }

    //    return this->searchText;

    static QString xd;

    return xd;
}

// Highlight
SBHighlight Message::getScrollBarHighlight() const
{
    if (this->hasFlags(Message::Highlighted)) {
        return SBHighlight(SBHighlight::Highlight);
    }
    return SBHighlight();
}

// Static
MessagePtr Message::createSystemMessage(const QString &text)
{
    MessagePtr message(new Message);

    message->addElement(new TimestampElement(QTime::currentTime()));
    message->addElement(new TextElement(text, MessageElement::Text, MessageColor::System));
    message->addFlags(Message::System);

    return MessagePtr(message);
}

MessagePtr Message::createTimeoutMessage(const QString &username, const QString &durationInSeconds,
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
        text.append(util::ParseTagString(reason));
        text.append("\"");
    }
    text.append(".");

    if (multipleTimes) {
        text.append(" (multiple times)");
    }

    MessagePtr message = Message::createSystemMessage(text);
    message->addFlags(Message::Timeout);
    message->timeoutUser = username;
    return message;
}

}  // namespace messages
}  // namespace chatterino
