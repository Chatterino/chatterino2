#pragma once

#include "common/Aliases.hpp"
#include "common/Outcome.hpp"
#include "messages/MessageBuilder.hpp"

#include <IrcMessage>
#include <QColor>
#include <QUrl>

#include <optional>

namespace chatterino {

class Badge;
class Channel;

class SharedMessageBuilder : public MessageBuilder
{
public:
    SharedMessageBuilder() = delete;

    explicit SharedMessageBuilder(Channel *_channel,
                                  const Communi::IrcPrivateMessage *_ircMessage,
                                  const MessageParseArgs &_args);

    explicit SharedMessageBuilder(Channel *_channel,
                                  const Communi::IrcMessage *_ircMessage,
                                  const MessageParseArgs &_args,
                                  QString content, bool isAction);

    QString userName;

    [[nodiscard]] virtual bool isIgnored() const;

    // triggerHighlights triggers any alerts or sounds parsed by parseHighlights
    virtual void triggerHighlights();
    virtual MessagePtr build() = 0;

    static std::pair<QString, QString> slashKeyValue(const QString &kvStr);

    // Parses "badges" tag which contains a comma separated list of key-value elements
    static std::vector<Badge> parseBadgeTag(const QVariantMap &tags);

    static QString stylizeUsername(const QString &username,
                                   const Message &message);

protected:
    virtual void parse();

    virtual void parseUsernameColor();

    virtual void parseUsername();

    virtual Outcome tryAppendEmote(const EmoteName &name)
    {
        (void)name;
        return Failure;
    }

    // parseHighlights only updates the visual state of the message, but leaves the playing of alerts and sounds to the triggerHighlights function
    virtual void parseHighlights();
    static void triggerHighlights(const QString &channelName, bool playSound,
                                  const std::optional<QUrl> &customSoundUrl,
                                  bool windowAlert);

    void appendChannelName();

    Channel *channel;
    const Communi::IrcMessage *ircMessage;
    MessageParseArgs args;
    const QVariantMap tags;
    QString originalMessage_;

    const bool action_{};

    QColor usernameColor_ = {153, 153, 153};

    bool highlightAlert_ = false;
    bool highlightSound_ = false;
    std::optional<QUrl> highlightSoundCustomUrl_{};
};

}  // namespace chatterino
