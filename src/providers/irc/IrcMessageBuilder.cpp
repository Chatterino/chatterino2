#include "providers/irc/IrcMessageBuilder.hpp"

#include "Application.hpp"
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
#include "util/IrcHelpers.hpp"
#include "widgets/Window.hpp"

namespace chatterino {

IrcMessageBuilder::IrcMessageBuilder(
    Channel *_channel, const Communi::IrcPrivateMessage *_ircMessage,
    const MessageParseArgs &_args)
    : SharedMessageBuilder(_channel, _ircMessage, _args)
{
    this->usernameColor_ = getApp()->themes->messages.textColors.system;
}

IrcMessageBuilder::IrcMessageBuilder(Channel *_channel,
                                     const Communi::IrcMessage *_ircMessage,
                                     const MessageParseArgs &_args,
                                     QString content, bool isAction)
    : SharedMessageBuilder(_channel, _ircMessage, _args, content, isAction)
{
    assert(false);
    this->usernameColor_ = getApp()->themes->messages.textColors.system;
}

MessagePtr IrcMessageBuilder::build()
{
    // PARSE
    this->parse();

    // PUSH ELEMENTS
    this->appendChannelName();

    this->emplace<TimestampElement>();

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
    this->emplace<IrcTextElement>(words.join(' '), MessageElementFlag::Text);
}

void IrcMessageBuilder::appendUsername()
{
    auto app = getApp();

    QString username = this->userName;
    this->message().loginName = username;

    // The full string that will be rendered in the chat widget
    QString usernameText = username;

    if (!this->action_)
    {
        usernameText += ":";
    }

    this->emplace<TextElement>(usernameText, MessageElementFlag::Username,
                               this->usernameColor_, FontStyle::ChatMediumBold)
        ->setLink({Link::UserInfo, this->message().displayName});
}

}  // namespace chatterino
