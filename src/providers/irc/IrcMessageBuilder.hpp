#pragma once

#include "common/Aliases.hpp"
#include "common/Outcome.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/twitch/TwitchBadge.hpp"

#include <IrcMessage>
#include <QString>
#include <QVariant>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class Channel;
class TwitchChannel;

class IrcMessageBuilder : public MessageBuilder
{
public:
    IrcMessageBuilder() = delete;

    explicit IrcMessageBuilder(Channel *_channel,
                               const Communi::IrcPrivateMessage *_ircMessage,
                               const MessageParseArgs &_args);
    explicit IrcMessageBuilder(Channel *_channel,
                               const Communi::IrcMessage *_ircMessage,
                               const MessageParseArgs &_args, QString content,
                               bool isAction);

    Channel *channel;
    const Communi::IrcMessage *ircMessage;
    MessageParseArgs args;

    QString userName;

    [[nodiscard]] bool isIgnored() const;
    // triggerHighlights triggers any alerts or sounds parsed by parseHighlights
    void triggerHighlights();
    MessagePtr build();

private:
    void appendChannelName();
    void parseUsernameColor();
    void parseUsername();
    void appendUsername();
    // parseHighlights only updates the visual state of the message, but leaves the playing of alerts and sounds to the triggerHighlights function
    void parseHighlights();

    boost::optional<EmotePtr> getTwitchBadge(const Badge &badge);
    void appendTwitchEmote(
        const QString &emote,
        std::vector<std::tuple<int, EmotePtr, EmoteName>> &vec,
        std::vector<int> &correctPositions);

    void addWords(const QStringList &words);
    void addTextOrEmoji(EmotePtr emote);
    void addTextOrEmoji(const QString &value);

    QString userId_;
    QColor usernameColor_;
    QString originalMessage_;
    bool senderIsBroadcaster{};

    const bool action_ = false;

    bool highlightAlert_ = false;
    bool highlightSound_ = false;

    QUrl highlightSoundUrl_;
};

}  // namespace chatterino
