#include "messages/Message.hpp"
#include "MessageElement.hpp"
#include "providers/twitch/PubsubActions.hpp"
#include "util/IrcHelpers.hpp"

using SBHighlight = chatterino::ScrollbarHighlight;

namespace chatterino {

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
    } else if (this->flags & Message::Subscription) {
        return SBHighlight(SBHighlight::Subscription);
    }
    return SBHighlight();
}

// Static
MessagePtr Message::createSystemMessage(const QString &text)
{
    MessagePtr message(new Message);

    message->addElement(new TimestampElement(QTime::currentTime()));
    message->addElement(new TextElement(text, MessageElement::Text, MessageColor::System));
    message->flags |= MessageFlags::System;
    message->flags |= MessageFlags::DoNotTriggerNotification;
    message->searchText = text;

    return message;
}

MessagePtr Message::createMessage(const QString &text)
{
    MessagePtr message(new Message);

    message->addElement(new TimestampElement(QTime::currentTime()));
    message->addElement(new TextElement(text, MessageElement::Text, MessageColor::Text));
    message->searchText = text;

    return message;
}

namespace {

QString makeDuration(int count, const QString &order)
{
    QString text;

    text.append(QString::number(count));
    text.append(" " + order);

    if (count > 1) {
        text.append("s");
    }

    return text;
}

QString makeDuration(uint32_t timeoutSeconds)
{
    int timeoutMinutes = floor(timeoutSeconds / 60);

    if (timeoutMinutes > 0 && timeoutSeconds % 60 == 0) {
        int timeoutHours = floor(timeoutMinutes / 60);

        if (timeoutHours > 0 && timeoutMinutes % 60 == 0) {
            int timeoutDays = floor(timeoutHours / 24);

            if (timeoutDays > 0 && timeoutHours % 24 == 0) {
                return makeDuration(timeoutDays, "day");
            }

            return makeDuration(timeoutHours, "hour");
        }

        return makeDuration(timeoutMinutes, "minute");
    }

    return makeDuration(timeoutSeconds, "second");
}

}  // namespace

MessagePtr Message::createTimeoutMessage(const QString &username, const QString &durationInSeconds,
                                         const QString &reason, bool multipleTimes)
{
    QString text;

    text.append(username);
    if (!durationInSeconds.isEmpty()) {
        text.append(" has been timed out");

        // TODO: Implement who timed the user out

        text.append(" for ");
        bool ok = true;
        int timeoutSeconds = durationInSeconds.toInt(&ok);
        if (ok) {
            text.append(makeDuration(timeoutSeconds));
        }
    } else {
        text.append(" has been permanently banned");
    }

    if (reason.length() > 0) {
        text.append(": \"");
        text.append(parseTagString(reason));
        text.append("\"");
    }
    text.append(".");

    if (multipleTimes) {
        text.append(" (multiple times)");
    }

    MessagePtr message = Message::createSystemMessage(text);
    message->flags.EnableFlag(MessageFlags::System);
    message->flags.EnableFlag(MessageFlags::Timeout);
    message->timeoutUser = username;
    return message;
}

MessagePtr Message::createTimeoutMessage(const BanAction &action, uint32_t count)
{
    MessagePtr msg(new Message);

    msg->addElement(new TimestampElement(QTime::currentTime()));
    msg->flags.EnableFlag(MessageFlags::System);
    msg->flags.EnableFlag(MessageFlags::Timeout);

    msg->timeoutUser = action.target.name;
    msg->count = count;

    QString text;

    if (action.isBan()) {
        if (action.reason.isEmpty()) {
            text = QString("%1 banned %2.")  //
                       .arg(action.source.name)
                       .arg(action.target.name);
        } else {
            text = QString("%1 banned %2: \"%3\".")  //
                       .arg(action.source.name)
                       .arg(action.target.name)
                       .arg(action.reason);
        }
    } else {
        if (action.reason.isEmpty()) {
            text = QString("%1 timed out %2 for %3.")  //
                       .arg(action.source.name)
                       .arg(action.target.name)
                       .arg(makeDuration(action.duration));
        } else {
            text = QString("%1 timed out %2 for %3: \"%4\".")  //
                       .arg(action.source.name)
                       .arg(action.target.name)
                       .arg(makeDuration(action.duration))
                       .arg(action.reason);
        }

        if (count > 1) {
            text.append(QString(" (%1 times)").arg(count));
        }
    }

    msg->addElement(new chatterino::TextElement(text, chatterino::MessageElement::Text,
                                                chatterino::MessageColor::System));
    msg->searchText = text;

    return msg;
}

MessagePtr Message::createUntimeoutMessage(const UnbanAction &action)
{
    MessagePtr msg(new Message);

    msg->addElement(new TimestampElement(QTime::currentTime()));
    msg->flags.EnableFlag(MessageFlags::System);
    msg->flags.EnableFlag(MessageFlags::Untimeout);

    msg->timeoutUser = action.target.name;

    QString text;

    if (action.wasBan()) {
        text = QString("%1 unbanned %2.")  //
                   .arg(action.source.name)
                   .arg(action.target.name);
    } else {
        text = QString("%1 untimedout %2.")  //
                   .arg(action.source.name)
                   .arg(action.target.name);
    }

    msg->addElement(new chatterino::TextElement(text, chatterino::MessageElement::Text,
                                                chatterino::MessageColor::System));
    msg->searchText = text;

    return msg;
}

}  // namespace chatterino
