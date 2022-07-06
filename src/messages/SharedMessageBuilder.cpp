#include "messages/SharedMessageBuilder.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "controllers/ignores/IgnoreController.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"
#include "messages/MessageElement.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Helpers.hpp"
#include "util/Qt.hpp"
#include "util/StreamerMode.hpp"

#include <QFileInfo>
#include <QMediaPlayer>

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

void SharedMessageBuilder::parse()
{
    this->parseUsernameColor();

    if (this->action_)
    {
        this->textColor_ = this->usernameColor_;
    }

    this->parseUsername();

    this->message().flags.set(MessageFlag::Collapsed);
}

// "foo/bar/baz,tri/hard" can be a valid badge-info tag
// In that case, valid map content should be 'split by slash' only once:
// {"foo": "bar/baz", "tri": "hard"}
std::pair<QString, QString> SharedMessageBuilder::slashKeyValue(
    const QString &kvStr)
{
    return {
        // part before first slash (index 0 of section)
        kvStr.section('/', 0, 0),
        // part after first slash (index 1 of section)
        kvStr.section('/', 1, -1),
    };
}

std::vector<Badge> SharedMessageBuilder::parseBadgeTag(const QVariantMap &tags)
{
    std::vector<Badge> b;

    auto badgesIt = tags.constFind("badges");
    if (badgesIt == tags.end())
    {
        return b;
    }

    auto badges = badgesIt.value().toString().split(',', Qt::SkipEmptyParts);

    for (const QString &badge : badges)
    {
        if (!badge.contains('/'))
        {
            continue;
        }

        auto pair = SharedMessageBuilder::slashKeyValue(badge);
        b.emplace_back(Badge{pair.first, pair.second});
    }

    return b;
}

bool SharedMessageBuilder::isIgnored() const
{
    return isIgnoredMessage({
        /*.message = */ this->originalMessage_,
    });
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
    if (getCSettings().isBlacklistedUser(this->ircMessage->nick()))
    {
        // Do nothing. We ignore highlights from this user.
        return;
    }

    auto badges = SharedMessageBuilder::parseBadgeTag(this->tags);
    auto [highlighted, highlightResult] = getIApp()->getHighlights()->check(
        this->args, badges, this->ircMessage->nick(), this->originalMessage_);

    if (!highlighted)
    {
        return;
    }

    // This message triggered one or more highlights, act upon the highlight result

    this->message().flags.set(MessageFlag::Highlighted);

    this->highlightAlert_ = highlightResult.alert;

    this->highlightSound_ = highlightResult.playSound;

    this->message().highlightColor = highlightResult.color;

    if (highlightResult.customSoundUrl)
    {
        this->highlightSoundUrl_ = highlightResult.customSoundUrl.get();
    }
    else
    {
        this->highlightSoundUrl_ = getFallbackHighlightSound();
    }

    if (highlightResult.showInMentions)
    {
        this->message().flags.set(MessageFlag::ShowInMentions);
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
    auto &&textColor = this->textColor_;

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
    Link link(Link::JumpToChannel, this->channel->getName());

    this->emplace<TextElement>(channelName, MessageElementFlag::ChannelName,
                               MessageColor::System)
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

    if (isInStreamerMode() && getSettings()->streamerModeMuteMentions)
    {
        // We are in streamer mode with muting mention sounds enabled. Do nothing.
        return;
    }

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
