#include "providers/irc/IrcMessageBuilder.hpp"

#include "Application.hpp"
#include "common/IrcColors.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/ignores/IgnoreController.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"
#include "messages/Message.hpp"
#include "providers/chatterino/ChatterinoBadges.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Helpers.hpp"
#include "util/IrcHelpers.hpp"
#include "widgets/Window.hpp"

namespace {

QRegularExpression IRC_COLOR_PARSE_REGEX(
    "(\u0003(\\d{1,2})?(,(\\d{1,2}))?|\u000f)",
    QRegularExpression::UseUnicodePropertiesOption);

}  // namespace

namespace chatterino {

IrcMessageBuilder::IrcMessageBuilder(
    Channel *_channel, const Communi::IrcPrivateMessage *_ircMessage,
    const MessageParseArgs &_args)
    : SharedMessageBuilder(_channel, _ircMessage, _args)
{
}

IrcMessageBuilder::IrcMessageBuilder(Channel *_channel,
                                     const Communi::IrcMessage *_ircMessage,
                                     const MessageParseArgs &_args,
                                     QString content, bool isAction)
    : SharedMessageBuilder(_channel, _ircMessage, _args, content, isAction)
{
    assert(false);
}

MessagePtr IrcMessageBuilder::build()
{
    // PARSE
    this->parse();
    this->usernameColor_ = getRandomColor(this->ircMessage->nick());

    // PUSH ELEMENTS
    this->appendChannelName();

    this->emplace<TimestampElement>(
        calculateMessageTimestamp(this->ircMessage));

    this->appendUsername();

    // words
    this->addWords(this->originalMessage_.split(' '));

    this->message().messageText = this->originalMessage_;
    this->message().searchText = this->message().localizedName + " " +
                                 this->userName + ": " + this->originalMessage_;

    // highlights
    this->parseHighlights();

    // highlighting incoming whispers if requested per setting
    if (this->args.isReceivedWhisper && getSettings()->highlightInlineWhispers)
    {
        this->message().flags.set(MessageFlag::HighlightedWhisper, true);
    }

    return this->release();
}

void IrcMessageBuilder::addWords(const QStringList &words)
{
    MessageColor defaultColorType = MessageColor::Text;
    auto defaultColor = defaultColorType.getColor(*getApp()->themes);
    QColor textColor = defaultColor;
    int fg = -1;
    int bg = -1;

    for (auto word : words)
    {
        if (word.isEmpty())
        {
            continue;
        }

        auto string = QString(word);

        // Actually just text
        auto linkString = this->matchLink(string);
        auto link = Link();

        if (!linkString.isEmpty())
        {
            this->addLink(string, linkString);
            continue;
        }

        // Does the word contain a color changer? If so, split on it.
        // Add color indicators, then combine into the same word with the color being changed

        auto i = IRC_COLOR_PARSE_REGEX.globalMatch(string);

        if (!i.hasNext())
        {
            this->emplace<TextElement>(string, MessageElementFlag::Text,
                                       textColor);
            continue;
        }

        int pos = 0;
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
                this->emplace<TextElement>(
                        string.mid(lastPos, match.capturedStart() - lastPos),
                        MessageElementFlag::Text, textColor)
                    ->setTrailingSpace(false);
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
        this->emplace<TextElement>(string.mid(lastPos),
                                   MessageElementFlag::Text, textColor);
    }

    this->message().elements.back()->setTrailingSpace(false);
}

void IrcMessageBuilder::appendUsername()
{
    QString username = this->userName;
    this->message().loginName = username;
    this->message().displayName = username;

    // The full string that will be rendered in the chat widget
    QString usernameText = username;

    if (!this->action_)
    {
        usernameText += ":";
    }

    this->emplace<TextElement>(usernameText, MessageElementFlag::Username,
                               this->usernameColor_, FontStyle::ChatMediumBold)
        ->setLink({Link::UserInfo, this->message().loginName});
}

}  // namespace chatterino
