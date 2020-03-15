#include "messages/SharedMessageBuilder.hpp"

#include "Application.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"
#include "messages/Message.hpp"
#include "messages/MessageElement.hpp"
#include "providers/twitch/TwitchCommon.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"

namespace chatterino {

namespace {

    QUrl getFallbackHighlightSound()
    {
        QString path = getSettings()->pathHighlightSound;
        bool fileExists = QFileInfo::exists(path) && QFileInfo(path).isFile();

        // Use fallback sound when checkbox is not checked
        // or custom file doesn't exist
        if (getSettings()->customHighlightSound && fileExists)
        {
            return QUrl::fromLocalFile(path);
        }
        else
        {
            return QUrl("qrc:/sounds/ping2.wav");
        }
    }

}  // namespace

SharedMessageBuilder::SharedMessageBuilder(
    Channel *_channel, const Communi::IrcPrivateMessage *_ircMessage,
    const MessageParseArgs &_args)
    : channel(_channel)
    , ircMessage(_ircMessage)
    , args(_args)
    , tags(this->ircMessage->tags())
    , originalMessage_(_ircMessage->content())
    , action_(_ircMessage->isAction())
{
}

SharedMessageBuilder::SharedMessageBuilder(
    Channel *_channel, const Communi::IrcMessage *_ircMessage,
    const MessageParseArgs &_args, QString content, bool isAction)
    : channel(_channel)
    , ircMessage(_ircMessage)
    , args(_args)
    , tags(this->ircMessage->tags())
    , originalMessage_(content)
    , action_(isAction)
{
}

namespace {

    QColor getRandomColor(const QString &v)
    {
        int colorSeed = 0;
        for (const auto &c : v)
        {
            colorSeed += c.digitValue();
        }
        const auto colorIndex = colorSeed % TWITCH_USERNAME_COLORS.size();
        return TWITCH_USERNAME_COLORS[colorIndex];
    }

}  // namespace

void SharedMessageBuilder::parse()
{
    this->parseUsernameColor();

    this->parseUsername();

    this->message().flags.set(MessageFlag::Collapsed);
}

bool SharedMessageBuilder::isIgnored() const
{
    // TODO(pajlada): Do we need to check if the phrase is valid first?
    auto phrases = getCSettings().ignoredMessages.readOnly();
    for (const auto &phrase : *phrases)
    {
        if (phrase.isBlock() && phrase.isMatch(this->originalMessage_))
        {
            qDebug() << "Blocking message because it contains ignored phrase"
                     << phrase.getPattern();
            return true;
        }
    }

    return false;
}

void SharedMessageBuilder::parseUsernameColor()
{
    if (getSettings()->colorizeNicknames)
    {
        this->usernameColor_ = getRandomColor(this->ircMessage->nick());
    }
}

void SharedMessageBuilder::parseUsername()
{
    // username
    this->userName = this->ircMessage->nick();

    this->message().loginName = this->userName;
}

void SharedMessageBuilder::parseHighlights()
{
    auto app = getApp();

    if (this->message().flags.has(MessageFlag::Subscription) &&
        getSettings()->enableSubHighlight)
    {
        if (getSettings()->enableSubHighlightTaskbar)
        {
            this->highlightAlert_ = true;
        }

        if (getSettings()->enableSubHighlightSound)
        {
            this->highlightSound_ = true;

            // Use custom sound if set, otherwise use fallback
            if (!getSettings()->subHighlightSoundUrl.getValue().isEmpty())
            {
                this->highlightSoundUrl_ =
                    QUrl(getSettings()->subHighlightSoundUrl.getValue());
            }
            else
            {
                this->highlightSoundUrl_ = getFallbackHighlightSound();
            }
        }

        this->message().flags.set(MessageFlag::Highlighted);
        this->message().highlightColor =
            ColorProvider::instance().color(ColorType::Subscription);

        // This message was a subscription.
        // Don't check for any other highlight phrases.
        return;
    }

    auto currentUser = app->accounts->twitch.getCurrent();

    QString currentUsername = currentUser->getUserName();

    if (getCSettings().isBlacklistedUser(this->ircMessage->nick()))
    {
        // Do nothing. We ignore highlights from this user.
        return;
    }

    // Highlight because it's a whisper
    if (this->args.isReceivedWhisper && getSettings()->enableWhisperHighlight)
    {
        if (getSettings()->enableWhisperHighlightTaskbar)
        {
            this->highlightAlert_ = true;
        }

        if (getSettings()->enableWhisperHighlightSound)
        {
            this->highlightSound_ = true;

            // Use custom sound if set, otherwise use fallback
            if (!getSettings()->whisperHighlightSoundUrl.getValue().isEmpty())
            {
                this->highlightSoundUrl_ =
                    QUrl(getSettings()->whisperHighlightSoundUrl.getValue());
            }
            else
            {
                this->highlightSoundUrl_ = getFallbackHighlightSound();
            }
        }

        this->message().highlightColor =
            ColorProvider::instance().color(ColorType::Whisper);

        /*
         * Do _NOT_ return yet, we might want to apply phrase/user name
         * highlights (which override whisper color/sound).
         */
    }

    // Highlight because of sender
    auto userHighlights = getCSettings().highlightedUsers.readOnly();
    for (const HighlightPhrase &userHighlight : *userHighlights)
    {
        if (!userHighlight.isMatch(this->ircMessage->nick()))
        {
            continue;
        }
        qDebug() << "Highlight because user" << this->ircMessage->nick()
                 << "sent a message";

        this->message().flags.set(MessageFlag::Highlighted);
        this->message().highlightColor = userHighlight.getColor();

        if (userHighlight.hasAlert())
        {
            this->highlightAlert_ = true;
        }

        if (userHighlight.hasSound())
        {
            this->highlightSound_ = true;
            // Use custom sound if set, otherwise use the fallback sound
            if (userHighlight.hasCustomSound())
            {
                this->highlightSoundUrl_ = userHighlight.getSoundUrl();
            }
            else
            {
                this->highlightSoundUrl_ = getFallbackHighlightSound();
            }
        }

        if (this->highlightAlert_ && this->highlightSound_)
        {
            /*
             * User name highlights "beat" highlight phrases: If a message has
             * all attributes (color, taskbar flashing, sound) set, highlight
             * phrases will not be checked.
             */
            return;
        }
    }

    if (this->ircMessage->nick() == currentUsername)
    {
        // Do nothing. Highlights cannot be triggered by yourself
        return;
    }

    // TODO: This vector should only be rebuilt upon highlights being changed
    // fourtf: should be implemented in the HighlightsController
    std::vector<HighlightPhrase> activeHighlights =
        getSettings()->highlightedMessages.cloneVector();

    if (getSettings()->enableSelfHighlight && currentUsername.size() > 0)
    {
        HighlightPhrase selfHighlight(
            currentUsername, getSettings()->enableSelfHighlightTaskbar,
            getSettings()->enableSelfHighlightSound, false, false,
            getSettings()->selfHighlightSoundUrl.getValue(),
            ColorProvider::instance().color(ColorType::SelfHighlight));
        activeHighlights.emplace_back(std::move(selfHighlight));
    }

    // Highlight because of message
    for (const HighlightPhrase &highlight : activeHighlights)
    {
        if (!highlight.isMatch(this->originalMessage_))
        {
            continue;
        }

        this->message().flags.set(MessageFlag::Highlighted);
        this->message().highlightColor = highlight.getColor();

        if (highlight.hasAlert())
        {
            this->highlightAlert_ = true;
        }

        // Only set highlightSound_ if it hasn't been set by username
        // highlights already.
        if (highlight.hasSound() && !this->highlightSound_)
        {
            this->highlightSound_ = true;

            // Use custom sound if set, otherwise use fallback sound
            if (highlight.hasCustomSound())
            {
                this->highlightSoundUrl_ = highlight.getSoundUrl();
            }
            else
            {
                this->highlightSoundUrl_ = getFallbackHighlightSound();
            }
        }

        if (this->highlightAlert_ && this->highlightSound_)
        {
            /*
             * Break once no further attributes (taskbar, sound) can be
             * applied.
             */
            break;
        }
    }
}

void SharedMessageBuilder::addTextOrEmoji(EmotePtr emote)
{
    this->emplace<EmoteElement>(emote, MessageElementFlag::EmojiAll);
}

void SharedMessageBuilder::addTextOrEmoji(const QString &string_)
{
    auto string = QString(string_);

    // Actually just text
    auto linkString = this->matchLink(string);
    auto link = Link();
    auto textColor = this->action_ ? MessageColor(this->usernameColor_)
                                   : MessageColor(MessageColor::Text);

    if (linkString.isEmpty())
    {
        if (string.startsWith('@'))
        {
            this->emplace<TextElement>(string, MessageElementFlag::BoldUsername,
                                       textColor, FontStyle::ChatMediumBold);
            this->emplace<TextElement>(
                string, MessageElementFlag::NonBoldUsername, textColor);
        }
        else
        {
            this->emplace<TextElement>(string, MessageElementFlag::Text,
                                       textColor);
        }
    }
    else
    {
        this->addLink(string, linkString);
    }
}

void SharedMessageBuilder::appendChannelName()
{
    QString channelName("#" + this->channel->getName());
    Link link(Link::Url, this->channel->getName() + "\n" + this->message().id);

    this->emplace<TextElement>(channelName, MessageElementFlag::ChannelName,
                               MessageColor::System)  //
        ->setLink(link);
}

inline QMediaPlayer *getPlayer()
{
    if (isGuiThread())
    {
        static auto player = new QMediaPlayer;
        return player;
    }
    else
    {
        return nullptr;
    }
}

void SharedMessageBuilder::triggerHighlights()
{
    static QUrl currentPlayerUrl;

    if (getCSettings().isMutedChannel(this->channel->getName()))
    {
        // Do nothing. Pings are muted in this channel.
        return;
    }

    bool hasFocus = (QApplication::focusWidget() != nullptr);
    bool resolveFocus = !hasFocus || getSettings()->highlightAlwaysPlaySound;

    if (this->highlightSound_ && resolveFocus)
    {
        if (auto player = getPlayer())
        {
            // update the media player url if necessary
            if (currentPlayerUrl != this->highlightSoundUrl_)
            {
                player->setMedia(this->highlightSoundUrl_);

                currentPlayerUrl = this->highlightSoundUrl_;
            }

            player->play();
        }
    }

    if (this->highlightAlert_)
    {
        getApp()->windows->sendAlert();
    }
}

}  // namespace chatterino
