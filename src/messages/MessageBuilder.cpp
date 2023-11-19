#include "MessageBuilder.hpp"

#include "Application.hpp"
#include "common/IrcColors.hpp"
#include "common/LinkParser.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/Image.hpp"
#include "messages/Message.hpp"
#include "messages/MessageColor.hpp"
#include "messages/MessageElement.hpp"
#include "providers/LinkResolver.hpp"
#include "providers/twitch/PubSubActions.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Theme.hpp"
#include "util/FormatTime.hpp"
#include "util/Qt.hpp"

#include <QDateTime>

namespace {

QRegularExpression IRC_COLOR_PARSE_REGEX(
    "(\u0003(\\d{1,2})?(,(\\d{1,2}))?|\u000f)",
    QRegularExpression::UseUnicodePropertiesOption);

QString formatUpdatedEmoteList(const QString &platform,
                               const std::vector<QString> &emoteNames,
                               bool isAdd, bool isFirstWord)
{
    QString text = "";
    if (isAdd)
    {
        text += isFirstWord ? "Added" : "added";
    }
    else
    {
        text += isFirstWord ? "Removed" : "removed";
    }

    if (emoteNames.size() == 1)
    {
        text += QString(" %1 emote ").arg(platform);
    }
    else
    {
        text += QString(" %1 %2 emotes ").arg(emoteNames.size()).arg(platform);
    }

    auto i = 0;
    for (const auto &emoteName : emoteNames)
    {
        i++;
        if (i > 1)
        {
            text += i == emoteNames.size() ? " and " : ", ";
        }
        text += emoteName;
    }

    text += ".";

    return text;
}

}  // namespace

namespace chatterino {

MessagePtr makeSystemMessage(const QString &text)
{
    return MessageBuilder(systemMessage, text).release();
}

MessagePtr makeSystemMessage(const QString &text, const QTime &time)
{
    return MessageBuilder(systemMessage, text, time).release();
}

EmotePtr makeAutoModBadge()
{
    return std::make_shared<Emote>(Emote{
        EmoteName{},
        ImageSet{Image::fromResourcePixmap(getResources().twitch.automod)},
        Tooltip{"AutoMod"},
        Url{"https://dashboard.twitch.tv/settings/moderation/automod"}});
}

MessagePtr makeAutomodInfoMessage(const AutomodInfoAction &action)
{
    auto builder = MessageBuilder();
    QString text("AutoMod: ");

    builder.emplace<TimestampElement>();
    builder.message().flags.set(MessageFlag::PubSub);

    // AutoMod shield badge
    builder.emplace<BadgeElement>(makeAutoModBadge(),
                                  MessageElementFlag::BadgeChannelAuthority);
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
    builder.message().loginName = "automod";
    builder.message().flags.set(MessageFlag::PubSub);
    builder.message().flags.set(MessageFlag::Timeout);
    builder.message().flags.set(MessageFlag::AutoMod);

    // AutoMod shield badge
    builder.emplace<BadgeElement>(makeAutoModBadge(),
                                  MessageElementFlag::BadgeChannelAuthority);
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
    builder2.message().flags.set(MessageFlag::Timeout);
    builder2.message().flags.set(MessageFlag::AutoMod);

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
    const QStringList textFragments =
        text.split(QRegularExpression("\\s"), Qt::SkipEmptyParts);
    for (const auto &word : textFragments)
    {
        LinkParser parser(word);
        if (parser.result())
        {
            this->addLink(*parser.result());
            continue;
        }

        this->emplace<TextElement>(word, MessageElementFlag::Text,
                                   MessageColor::System);
    }
    this->message().flags.set(MessageFlag::System);
    this->message().flags.set(MessageFlag::DoNotTriggerNotification);
    this->message().messageText = text;
    this->message().searchText = text;
}

MessageBuilder::MessageBuilder(TimeoutMessageTag, const QString &timeoutUser,
                               const QString &sourceUser,
                               const QString &systemMessageText, int times,
                               const QTime &time)
    : MessageBuilder()
{
    QString usernameText = systemMessageText.split(" ").at(0);
    QString remainder = systemMessageText.mid(usernameText.length() + 1);
    bool timeoutUserIsFirst =
        usernameText == "You" || timeoutUser == usernameText;
    QString messageText;

    this->emplace<TimestampElement>(time);
    this->emplaceSystemTextAndUpdate(usernameText, messageText)
        ->setLink(
            {Link::UserInfo, timeoutUserIsFirst ? timeoutUser : sourceUser});

    if (!sourceUser.isEmpty())
    {
        // the second username in the message
        const auto &targetUsername =
            timeoutUserIsFirst ? sourceUser : timeoutUser;
        int userPos = remainder.indexOf(targetUsername);

        QString mid = remainder.mid(0, userPos - 1);
        QString username = remainder.mid(userPos, targetUsername.length());
        remainder = remainder.mid(userPos + targetUsername.length() + 1);

        this->emplaceSystemTextAndUpdate(mid, messageText);
        this->emplaceSystemTextAndUpdate(username, messageText)
            ->setLink({Link::UserInfo, username});
    }

    this->emplaceSystemTextAndUpdate(
        QString("%1 (%2 times)").arg(remainder.trimmed()).arg(times),
        messageText);

    this->message().messageText = messageText;
    this->message().searchText = messageText;
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
    this->message().loginName = action.source.login;
    this->message().count = count;

    QString text;

    if (action.target.id == current->getUserId())
    {
        this->emplaceSystemTextAndUpdate("You", text)
            ->setLink({Link::UserInfo, current->getUserName()});
        this->emplaceSystemTextAndUpdate("were", text);
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
            text = QString("%1 added \"%2\" as a permitted term on AutoMod.")
                       .arg(action.source.login, action.message);
        }
        break;

        case AutomodUserAction::AddBlocked: {
            text = QString("%1 added \"%2\" as a blocked term on AutoMod.")
                       .arg(action.source.login, action.message);
        }
        break;

        case AutomodUserAction::RemovePermitted: {
            text = QString("%1 removed \"%2\" as a permitted term on AutoMod.")
                       .arg(action.source.login, action.message);
        }
        break;

        case AutomodUserAction::RemoveBlocked: {
            text = QString("%1 removed \"%2\" as a blocked term on AutoMod.")
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

MessageBuilder::MessageBuilder(LiveUpdatesAddEmoteMessageTag /*unused*/,
                               const QString &platform, const QString &actor,
                               const std::vector<QString> &emoteNames)
    : MessageBuilder()
{
    auto text =
        formatUpdatedEmoteList(platform, emoteNames, true, actor.isEmpty());

    this->emplace<TimestampElement>();
    if (!actor.isEmpty())
    {
        this->emplace<TextElement>(actor, MessageElementFlag::Username,
                                   MessageColor::System)
            ->setLink({Link::UserInfo, actor});
    }
    this->emplace<TextElement>(text, MessageElementFlag::Text,
                               MessageColor::System);

    QString finalText;
    if (actor.isEmpty())
    {
        finalText = text;
    }
    else
    {
        finalText = QString("%1 %2").arg(actor, text);
    }

    this->message().loginName = actor;
    this->message().messageText = finalText;
    this->message().searchText = finalText;

    this->message().flags.set(MessageFlag::System);
    this->message().flags.set(MessageFlag::LiveUpdatesAdd);
    this->message().flags.set(MessageFlag::DoNotTriggerNotification);
}

MessageBuilder::MessageBuilder(LiveUpdatesRemoveEmoteMessageTag /*unused*/,
                               const QString &platform, const QString &actor,
                               const std::vector<QString> &emoteNames)
    : MessageBuilder()
{
    auto text =
        formatUpdatedEmoteList(platform, emoteNames, false, actor.isEmpty());

    this->emplace<TimestampElement>();
    if (!actor.isEmpty())
    {
        this->emplace<TextElement>(actor, MessageElementFlag::Username,
                                   MessageColor::System)
            ->setLink({Link::UserInfo, actor});
    }
    this->emplace<TextElement>(text, MessageElementFlag::Text,
                               MessageColor::System);

    QString finalText;
    if (actor.isEmpty())
    {
        finalText = text;
    }
    else
    {
        finalText = QString("%1 %2").arg(actor, text);
    }

    this->message().loginName = actor;
    this->message().messageText = finalText;
    this->message().searchText = finalText;

    this->message().flags.set(MessageFlag::System);
    this->message().flags.set(MessageFlag::LiveUpdatesRemove);
    this->message().flags.set(MessageFlag::DoNotTriggerNotification);
}

MessageBuilder::MessageBuilder(LiveUpdatesUpdateEmoteMessageTag /*unused*/,
                               const QString &platform, const QString &actor,
                               const QString &emoteName,
                               const QString &oldEmoteName)
    : MessageBuilder()
{
    QString text;
    if (actor.isEmpty())
    {
        text = "Renamed";
    }
    else
    {
        text = "renamed";
    }
    text +=
        QString(" %1 emote %2 to %3.").arg(platform, oldEmoteName, emoteName);

    this->emplace<TimestampElement>();
    if (!actor.isEmpty())
    {
        this->emplace<TextElement>(actor, MessageElementFlag::Username,
                                   MessageColor::System)
            ->setLink({Link::UserInfo, actor});
    }
    this->emplace<TextElement>(text, MessageElementFlag::Text,
                               MessageColor::System);

    QString finalText;
    if (actor.isEmpty())
    {
        finalText = text;
    }
    else
    {
        finalText = QString("%1 %2").arg(actor, text);
    }

    this->message().loginName = actor;
    this->message().messageText = finalText;
    this->message().searchText = finalText;

    this->message().flags.set(MessageFlag::System);
    this->message().flags.set(MessageFlag::LiveUpdatesUpdate);
    this->message().flags.set(MessageFlag::DoNotTriggerNotification);
}

MessageBuilder::MessageBuilder(LiveUpdatesUpdateEmoteSetMessageTag /*unused*/,
                               const QString &platform, const QString &actor,
                               const QString &emoteSetName)
    : MessageBuilder()
{
    auto text = QString("switched the active %1 Emote Set to \"%2\".")
                    .arg(platform, emoteSetName);

    this->emplace<TimestampElement>();
    this->emplace<TextElement>(actor, MessageElementFlag::Username,
                               MessageColor::System)
        ->setLink({Link::UserInfo, actor});
    this->emplace<TextElement>(text, MessageElementFlag::Text,
                               MessageColor::System);

    auto finalText = QString("%1 %2").arg(actor, text);

    this->message().loginName = actor;
    this->message().messageText = finalText;
    this->message().searchText = finalText;

    this->message().flags.set(MessageFlag::System);
    this->message().flags.set(MessageFlag::LiveUpdatesUpdate);
    this->message().flags.set(MessageFlag::DoNotTriggerNotification);
}

MessageBuilder::MessageBuilder(ImageUploaderResultTag /*unused*/,
                               const QString &imageLink,
                               const QString &deletionLink,
                               size_t imagesStillQueued, size_t secondsLeft)
    : MessageBuilder()
{
    this->message().flags.set(MessageFlag::System);
    this->message().flags.set(MessageFlag::DoNotTriggerNotification);

    this->emplace<TimestampElement>();

    using MEF = MessageElementFlag;
    auto addText = [this](QString text, MessageElementFlags mefs = MEF::Text,
                          MessageColor color =
                              MessageColor::System) -> TextElement * {
        this->message().searchText += text;
        this->message().messageText += text;
        return this->emplace<TextElement>(text, mefs, color);
    };

    addText("Your image has been uploaded to");

    // ASSUMPTION: the user gave this uploader configuration to the program
    // therefore they trust that the host is not wrong/malicious. This doesn't obey getSettings()->lowercaseDomains.
    // This also ensures that the LinkResolver doesn't get these links.
    addText(imageLink, {MEF::OriginalLink, MEF::LowercaseLink},
            MessageColor::Link)
        ->setLink({Link::Url, imageLink})
        ->setTrailingSpace(false);

    if (!deletionLink.isEmpty())
    {
        addText("(Deletion link:");
        addText(deletionLink, {MEF::OriginalLink, MEF::LowercaseLink},
                MessageColor::Link)
            ->setLink({Link::Url, deletionLink})
            ->setTrailingSpace(false);
        addText(")")->setTrailingSpace(false);
    }
    addText(".");

    if (imagesStillQueued == 0)
    {
        return;
    }

    addText(QString("%1 left. Please wait until all of them are uploaded. "
                    "About %2 seconds left.")
                .arg(imagesStillQueued)
                .arg(secondsLeft));
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

bool MessageBuilder::isEmpty() const
{
    return this->message_->elements.empty();
}

MessageElement &MessageBuilder::back()
{
    assert(!this->isEmpty());
    return *this->message().elements.back();
}

std::unique_ptr<MessageElement> MessageBuilder::releaseBack()
{
    assert(!this->isEmpty());

    auto ptr = std::move(this->message().elements.back());
    this->message().elements.pop_back();
    return ptr;
}

void MessageBuilder::addLink(const ParsedLink &parsedLink)
{
    QString lowercaseLinkString;
    QString origLink = parsedLink.source;
    QString matchedLink;

    if (parsedLink.protocol.isNull())
    {
        matchedLink = QStringLiteral("http://") + parsedLink.source;
    }
    else
    {
        lowercaseLinkString += parsedLink.protocol;
        matchedLink = parsedLink.source;
    }

    lowercaseLinkString += parsedLink.host.toString().toLower();
    lowercaseLinkString += parsedLink.rest;

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

void MessageBuilder::addIrcMessageText(const QString &text)
{
    this->message().messageText = text;

    auto words = text.split(' ');
    MessageColor defaultColorType = MessageColor::Text;
    const auto &defaultColor = defaultColorType.getColor(*getApp()->themes);
    QColor textColor = defaultColor;
    int fg = -1;
    int bg = -1;

    for (const auto &word : words)
    {
        if (word.isEmpty())
        {
            continue;
        }

        auto string = QString(word);

        // Actually just text
        LinkParser parser(string);
        if (parser.result())
        {
            this->addLink(*parser.result());
            continue;
        }

        // Does the word contain a color changer? If so, split on it.
        // Add color indicators, then combine into the same word with the color being changed

        auto i = IRC_COLOR_PARSE_REGEX.globalMatch(string);

        if (!i.hasNext())
        {
            this->addIrcWord(string, textColor);
            continue;
        }

        int lastPos = 0;

        while (i.hasNext())
        {
            auto match = i.next();

            if (lastPos != match.capturedStart() && match.capturedStart() != 0)
            {
                if (fg >= 0 && fg <= 98)
                {
                    textColor = IRC_COLORS[fg];
                    getApp()->themes->normalizeColor(textColor);
                }
                else
                {
                    textColor = defaultColor;
                }
                this->addIrcWord(
                    string.mid(lastPos, match.capturedStart() - lastPos),
                    textColor, false);
                lastPos = match.capturedStart() + match.capturedLength();
            }
            if (!match.captured(1).isEmpty())
            {
                fg = -1;
                bg = -1;
            }

            if (!match.captured(2).isEmpty())
            {
                fg = match.captured(2).toInt(nullptr);
            }
            else
            {
                fg = -1;
            }
            if (!match.captured(4).isEmpty())
            {
                bg = match.captured(4).toInt(nullptr);
            }
            else if (fg == -1)
            {
                bg = -1;
            }

            lastPos = match.capturedStart() + match.capturedLength();
        }

        if (fg >= 0 && fg <= 98)
        {
            textColor = IRC_COLORS[fg];
            getApp()->themes->normalizeColor(textColor);
        }
        else
        {
            textColor = defaultColor;
        }
        this->addIrcWord(string.mid(lastPos), textColor);
    }

    this->message().elements.back()->setTrailingSpace(false);
}

void MessageBuilder::addTextOrEmoji(EmotePtr emote)
{
    this->emplace<EmoteElement>(emote, MessageElementFlag::EmojiAll);
}

void MessageBuilder::addTextOrEmoji(const QString &string_)
{
    auto string = QString(string_);

    // Actually just text
    LinkParser linkParser(string);
    if (linkParser.result())
    {
        this->addLink(*linkParser.result());
        return;
    }

    auto &&textColor = this->textColor_;
    if (string.startsWith('@'))
    {
        this->emplace<TextElement>(string, MessageElementFlag::BoldUsername,
                                   textColor, FontStyle::ChatMediumBold);
        this->emplace<TextElement>(string, MessageElementFlag::NonBoldUsername,
                                   textColor);
    }
    else
    {
        this->emplace<TextElement>(string, MessageElementFlag::Text, textColor);
    }
}

void MessageBuilder::addIrcWord(const QString &text, const QColor &color,
                                bool addSpace)
{
    this->textColor_ = color;
    for (auto &variant : getApp()->emotes->emojis.parse(text))
    {
        boost::apply_visitor(
            [&](auto &&arg) {
                this->addTextOrEmoji(arg);
            },
            variant);
        if (!addSpace)
        {
            this->message().elements.back()->setTrailingSpace(false);
        }
    }
}

TextElement *MessageBuilder::emplaceSystemTextAndUpdate(const QString &text,
                                                        QString &toUpdate)
{
    toUpdate.append(text + " ");
    return this->emplace<TextElement>(text, MessageElementFlag::Text,
                                      MessageColor::System);
}

}  // namespace chatterino
