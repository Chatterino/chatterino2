#include "messages/message.hpp"
#include "messageelement.hpp"
#include "util/irchelpers.hpp"

typedef chatterino::widgets::ScrollbarHighlight SBHighlight;

namespace chatterino {
namespace messages {
void Message::addElement(MessageElement *element)
{
    this->elements.push_back(std::unique_ptr<MessageElement>(element));
}

const std::vector<std::unique_ptr<MessageElement>> &Message::getElements() const
{
    return this->elements;
}

SBHighlight Message::getScrollBarHighlight() const
{
    if (this->flags & Message::Highlighted) {
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
    message->flags &= Message::System;
    message->searchText = text;

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
    message->flags &= Message::Timeout;
    message->timeoutUser = username;
    return message;
}

}  // namespace messages
}  // namespace chatterino
