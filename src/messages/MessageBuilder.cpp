#include "MessageBuilder.hpp"

#include "Application.hpp"
#include "common/LinkParser.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/Image.hpp"
#include "messages/Message.hpp"
#include "messages/MessageElement.hpp"
#include "providers/LinkResolver.hpp"
#include "providers/twitch/PubsubActions.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Theme.hpp"
#include "util/FormatTime.hpp"

#include <QDateTime>
#include <QImageReader>

namespace chatterino {

MessagePtr makeSystemMessage(const QString &text)
{
    return MessageBuilder(systemMessage, text).release();
}

MessagePtr makeSystemMessage(const QString &text, const QTime &time)
{
    return MessageBuilder(systemMessage, text, time).release();
}

MessagePtr makeAutomodInfoMessage(const AutomodInfoAction &action)
{
    auto builder = MessageBuilder();
    QString text("AutoMod: ");

    builder.emplace<TimestampElement>();
    builder.message().flags.set(MessageFlag::PubSub);

    // AutoMod shield badge
    builder
        .emplace<ImageElement>(Image::fromPixmap(getResources().twitch.automod),
                               MessageElementFlag::BadgeChannelAuthority)
        ->setTooltip("AutoMod");
    // AutoMod "username"
    builder.emplace<TextElement>("AutoMod:", MessageElementFlag::BoldUsername,
                                 MessageColor(QColor("blue")),
                                 FontStyle::ChatMediumBold);
    builder.emplace<TextElement>(
        "AutoMod:", MessageElementFlag::NonBoldUsername,
        MessageColor(QColor("blue")));
    switch (action.type)
    {
        case AutomodInfoAction::OnHold: {
            QString info("Hey! Your message is being checked "
                         "by mods and has not been sent.");
            text += info;
            builder.emplace<TextElement>(info, MessageElementFlag::Text,
                                         MessageColor::Text);
        }
        break;
        case AutomodInfoAction::Denied: {
            QString info("Mods have removed your message.");
            text += info;
            builder.emplace<TextElement>(info, MessageElementFlag::Text,
                                         MessageColor::Text);
        }
        break;
        case AutomodInfoAction::Approved: {
            QString info("Mods have accepted your message.");
            text += info;
            builder.emplace<TextElement>(info, MessageElementFlag::Text,
                                         MessageColor::Text);
        }
        break;
    }

    builder.message().flags.set(MessageFlag::AutoMod);
    builder.message().messageText = text;
    builder.message().searchText = text;

    auto message = builder.release();

    return message;
}

std::pair<MessagePtr, MessagePtr> makeAutomodMessage(
    const AutomodAction &action)
{
    MessageBuilder builder, builder2;

    //
    // Builder for AutoMod message with explanation
    builder.emplace<TimestampElement>();
    builder.message().loginName = "automod";
    builder.message().flags.set(MessageFlag::PubSub);

    // AutoMod shield badge
    builder
        .emplace<ImageElement>(Image::fromPixmap(getResources().twitch.automod),
                               MessageElementFlag::BadgeChannelAuthority)
        ->setTooltip("AutoMod");
    // AutoMod "username"
    builder.emplace<TextElement>("AutoMod:", MessageElementFlag::BoldUsername,
                                 MessageColor(QColor("blue")),
                                 FontStyle::ChatMediumBold);
    builder.emplace<TextElement>(
        "AutoMod:", MessageElementFlag::NonBoldUsername,
        MessageColor(QColor("blue")));
    // AutoMod header message
    builder.emplace<TextElement>(
        ("Held a message for reason: " + action.reason +
         ". Allow will post it in chat. "),
        MessageElementFlag::Text, MessageColor::Text);
    // Allow link button
    builder
        .emplace<TextElement>("Allow", MessageElementFlag::Text,
                              MessageColor(QColor("green")),
                              FontStyle::ChatMediumBold)
        ->setLink({Link::AutoModAllow, action.msgID});
    // Deny link button
    builder
        .emplace<TextElement>(" Deny", MessageElementFlag::Text,
                              MessageColor(QColor("red")),
                              FontStyle::ChatMediumBold)
        ->setLink({Link::AutoModDeny, action.msgID});
    // ID of message caught by AutoMod
    //    builder.emplace<TextElement>(action.msgID, MessageElementFlag::Text,
    //                                 MessageColor::Text);
    builder.message().flags.set(MessageFlag::AutoMod);
    auto text1 =
        QString("AutoMod: Held a message for reason: %1. Allow will post "
                "it in chat. Allow Deny")
            .arg(action.reason);
    builder.message().messageText = text1;
    builder.message().searchText = text1;

    auto message1 = builder.release();

    //
    // Builder for offender's message
    builder2.emplace<TimestampElement>();
    builder2.emplace<TwitchModerationElement>();
    builder2.message().loginName = action.target.login;
    builder2.message().flags.set(MessageFlag::PubSub);

    // sender username
    builder2
        .emplace<TextElement>(
            action.target.displayName + ":", MessageElementFlag::BoldUsername,
            MessageColor(action.target.color), FontStyle::ChatMediumBold)
        ->setLink({Link::UserInfo, action.target.login});
    builder2
        .emplace<TextElement>(action.target.displayName + ":",
                              MessageElementFlag::NonBoldUsername,
                              MessageColor(action.target.color))
        ->setLink({Link::UserInfo, action.target.login});
    // sender's message caught by AutoMod
    builder2.emplace<TextElement>(action.message, MessageElementFlag::Text,
                                  MessageColor::Text);
    builder2.message().flags.set(MessageFlag::AutoMod);
    auto text2 =
        QString("%1: %2").arg(action.target.displayName, action.message);
    builder2.message().messageText = text2;
    builder2.message().searchText = text2;

    auto message2 = builder2.release();

    return std::make_pair(message1, message2);
}

MessageBuilder::MessageBuilder()
    : message_(std::make_shared<Message>())
{
}

MessageBuilder::MessageBuilder(SystemMessageTag, const QString &text,
                               const QTime &time)
    : MessageBuilder()
{
    this->emplace<TimestampElement>(time);

    // check system message for links
    // (e.g. needed for sub ticket message in sub only mode)
    const QStringList textFragments = text.split(QRegularExpression("\\s"));
    for (const auto &word : textFragments)
    {
        const auto linkString = this->matchLink(word);
        if (linkString.isEmpty())
        {
            this->emplace<TextElement>(word, MessageElementFlag::Text,
                                       MessageColor::System);
        }
        else
        {
            this->addLink(word, linkString);
        }
    }
    this->message().flags.set(MessageFlag::System);
    this->message().flags.set(MessageFlag::DoNotTriggerNotification);
    this->message().messageText = text;
    this->message().searchText = text;
}

MessageBuilder::MessageBuilder(TimeoutMessageTag,
                               const QString &systemMessageText, int times,
                               const QTime &time)
    : MessageBuilder()
{
    QString username = systemMessageText.split(" ").at(0);
    QString remainder = systemMessageText.mid(username.length() + 1);

    QString text;

    this->emplace<TimestampElement>(time);
    this->emplaceSystemTextAndUpdate(username, text)
        ->setLink({Link::UserInfo, username});
    this->emplaceSystemTextAndUpdate(
        QString("%1 (%2 times)").arg(remainder.trimmed()).arg(times), text);

    this->message().messageText = text;
    this->message().searchText = text;
}

MessageBuilder::MessageBuilder(TimeoutMessageTag, const QString &username,
                               const QString &durationInSeconds,
                               bool multipleTimes, const QTime &time)
    : MessageBuilder()
{
    QString fullText;
    QString text;

    this->emplace<TimestampElement>(time);
    this->emplaceSystemTextAndUpdate(username, fullText)
        ->setLink({Link::UserInfo, username});

    if (!durationInSeconds.isEmpty())
    {
        text.append("has been timed out");

        // TODO: Implement who timed the user out

        text.append(" for ");
        bool ok = true;
        int timeoutSeconds = durationInSeconds.toInt(&ok);
        if (ok)
        {
            text.append(formatTime(timeoutSeconds));
        }
    }
    else
    {
        text.append("has been permanently banned");
    }

    text.append(".");

    if (multipleTimes)
    {
        text.append(" (multiple times)");
    }

    this->message().flags.set(MessageFlag::System);
    this->message().flags.set(MessageFlag::Timeout);
    this->message().flags.set(MessageFlag::DoNotTriggerNotification);
    this->message().timeoutUser = username;

    this->emplaceSystemTextAndUpdate(text, fullText);
    this->message().messageText = fullText;
    this->message().searchText = fullText;
}

// XXX: This does not belong in the MessageBuilder, this should be part of the TwitchMessageBuilder
MessageBuilder::MessageBuilder(const BanAction &action, uint32_t count)
    : MessageBuilder()
{
    auto current = getApp()->accounts->twitch.getCurrent();

    this->emplace<TimestampElement>();
    this->message().flags.set(MessageFlag::System);
    this->message().flags.set(MessageFlag::Timeout);
    this->message().timeoutUser = action.target.login;
    this->message().count = count;

    QString text;

    if (action.target.id == current->getUserId())
    {
        this->emplaceSystemTextAndUpdate("You were", text);
        if (action.isBan())
        {
            this->emplaceSystemTextAndUpdate("banned", text);
        }
        else
        {
            this->emplaceSystemTextAndUpdate(
                QString("timed out for %1").arg(formatTime(action.duration)),
                text);
        }

        if (!action.source.login.isEmpty())
        {
            this->emplaceSystemTextAndUpdate("by", text);
            this->emplaceSystemTextAndUpdate(
                    action.source.login + (action.reason.isEmpty() ? "." : ":"),
                    text)
                ->setLink({Link::UserInfo, action.source.login});
        }

        if (!action.reason.isEmpty())
        {
            this->emplaceSystemTextAndUpdate(
                QString("\"%1\".").arg(action.reason), text);
        }
    }
    else
    {
        if (action.isBan())
        {
            this->emplaceSystemTextAndUpdate(action.source.login, text)
                ->setLink({Link::UserInfo, action.source.login});
            this->emplaceSystemTextAndUpdate("banned", text);
            if (action.reason.isEmpty())
            {
                this->emplaceSystemTextAndUpdate(action.target.login, text)
                    ->setLink({Link::UserInfo, action.target.login});
            }
            else
            {
                this->emplaceSystemTextAndUpdate(action.target.login + ":",
                                                 text)
                    ->setLink({Link::UserInfo, action.target.login});
                this->emplaceSystemTextAndUpdate(
                    QString("\"%1\".").arg(action.reason), text);
            }
        }
        else
        {
            this->emplaceSystemTextAndUpdate(action.source.login, text)
                ->setLink({Link::UserInfo, action.source.login});
            this->emplaceSystemTextAndUpdate("timed out", text);
            this->emplaceSystemTextAndUpdate(action.target.login, text)
                ->setLink({Link::UserInfo, action.target.login});
            if (action.reason.isEmpty())
            {
                this->emplaceSystemTextAndUpdate(
                    QString("for %1.").arg(formatTime(action.duration)), text);
            }
            else
            {
                this->emplaceSystemTextAndUpdate(
                    QString("for %1: \"%2\".")
                        .arg(formatTime(action.duration))
                        .arg(action.reason),
                    text);
            }

            if (count > 1)
            {
                this->emplaceSystemTextAndUpdate(
                    QString("(%1 times)").arg(count), text);
            }
        }
    }

    this->message().messageText = text;
    this->message().searchText = text;
}

MessageBuilder::MessageBuilder(const UnbanAction &action)
    : MessageBuilder()
{
    this->emplace<TimestampElement>();
    this->message().flags.set(MessageFlag::System);
    this->message().flags.set(MessageFlag::Untimeout);

    this->message().timeoutUser = action.target.login;

    QString text;

    this->emplaceSystemTextAndUpdate(action.source.login, text)
        ->setLink({Link::UserInfo, action.source.login});
    this->emplaceSystemTextAndUpdate(
        action.wasBan() ? "unbanned" : "untimedout", text);
    this->emplaceSystemTextAndUpdate(action.target.login, text)
        ->setLink({Link::UserInfo, action.target.login});

    this->message().messageText = text;
    this->message().searchText = text;
}

MessageBuilder::MessageBuilder(const AutomodUserAction &action)
    : MessageBuilder()
{
    this->emplace<TimestampElement>();
    this->message().flags.set(MessageFlag::System);

    QString text;
    switch (action.type)
    {
        case AutomodUserAction::AddPermitted: {
            text = QString("%1 added %2 as a permitted term on AutoMod.")
                       .arg(action.source.login, action.message);
        }
        break;

        case AutomodUserAction::AddBlocked: {
            text = QString("%1 added %2 as a blocked term on AutoMod.")
                       .arg(action.source.login, action.message);
        }
        break;

        case AutomodUserAction::RemovePermitted: {
            text = QString("%1 removed %2 as a permitted term on AutoMod.")
                       .arg(action.source.login, action.message);
        }
        break;

        case AutomodUserAction::RemoveBlocked: {
            text = QString("%1 removed %2 as a blocked term on AutoMod.")
                       .arg(action.source.login, action.message);
        }
        break;

        case AutomodUserAction::Properties: {
            text = QString("%1 modified the AutoMod properties.")
                       .arg(action.source.login);
        }
        break;
    }
    this->message().messageText = text;
    this->message().searchText = text;

    this->emplace<TextElement>(text, MessageElementFlag::Text,
                               MessageColor::System);
}

Message *MessageBuilder::operator->()
{
    return this->message_.get();
}

Message &MessageBuilder::message()
{
    return *this->message_;
}

MessagePtr MessageBuilder::release()
{
    std::shared_ptr<Message> ptr;
    this->message_.swap(ptr);
    return ptr;
}

std::weak_ptr<Message> MessageBuilder::weakOf()
{
    return this->message_;
}

void MessageBuilder::append(std::unique_ptr<MessageElement> element)
{
    this->message().elements.push_back(std::move(element));
}

QString MessageBuilder::matchLink(const QString &string)
{
    LinkParser linkParser(string);

    static QRegularExpression httpRegex(
        "\\bhttps?://", QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression ftpRegex(
        "\\bftps?://", QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression spotifyRegex(
        "\\bspotify:", QRegularExpression::CaseInsensitiveOption);

    if (!linkParser.hasMatch())
    {
        return QString();
    }

    QString captured = linkParser.getCaptured();

    if (!captured.contains(httpRegex) && !captured.contains(ftpRegex) &&
        !captured.contains(spotifyRegex))
    {
        captured.insert(0, "http://");
    }

    return captured;
}

void MessageBuilder::addLink(const QString &origLink,
                             const QString &matchedLink)
{
    static QRegularExpression domainRegex(
        R"(^(?:(?:ftp|http)s?:\/\/)?([^\/]+)(?:\/.*)?$)",
        QRegularExpression::CaseInsensitiveOption);

    QString lowercaseLinkString;
    auto match = domainRegex.match(origLink);
    if (match.isValid())
    {
        lowercaseLinkString = origLink.mid(0, match.capturedStart(1)) +
                              match.captured(1).toLower() +
                              origLink.mid(match.capturedEnd(1));
    }
    else
    {
        lowercaseLinkString = origLink;
    }
    auto linkElement = Link(Link::Url, matchedLink);

    auto textColor = MessageColor(MessageColor::Link);
    auto linkMELowercase =
        this->emplace<TextElement>(lowercaseLinkString,
                                   MessageElementFlag::LowercaseLink, textColor)
            ->setLink(linkElement);
    auto linkMEOriginal =
        this->emplace<TextElement>(origLink, MessageElementFlag::OriginalLink,
                                   textColor)
            ->setLink(linkElement);

    LinkResolver::getLinkInfo(
        matchedLink, nullptr,
        [weakMessage = this->weakOf(), linkMELowercase, linkMEOriginal,
         matchedLink](QString tooltipText, Link originalLink,
                      ImagePtr thumbnail) {
            auto shared = weakMessage.lock();
            if (!shared)
            {
                return;
            }
            if (!tooltipText.isEmpty())
            {
                linkMELowercase->setTooltip(tooltipText);
                linkMEOriginal->setTooltip(tooltipText);
            }
            if (originalLink.value != matchedLink &&
                !originalLink.value.isEmpty())
            {
                linkMELowercase->setLink(originalLink)->updateLink();
                linkMEOriginal->setLink(originalLink)->updateLink();
            }
            linkMELowercase->setThumbnail(thumbnail);
            linkMELowercase->setThumbnailType(
                MessageElement::ThumbnailType::Link_Thumbnail);
            linkMEOriginal->setThumbnail(thumbnail);
            linkMEOriginal->setThumbnailType(
                MessageElement::ThumbnailType::Link_Thumbnail);
        });
}

TextElement *MessageBuilder::emplaceSystemTextAndUpdate(const QString &text,
                                                        QString &toUpdate)
{
    toUpdate.append(text + " ");
    return this->emplace<TextElement>(text, MessageElementFlag::Text,
                                      MessageColor::System);
}

}  // namespace chatterino
