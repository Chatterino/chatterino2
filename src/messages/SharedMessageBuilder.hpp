#include "messages/MessageBuilder.hpp"

#include "common/Aliases.hpp"
#include "common/Outcome.hpp"
#include "messages/MessageColor.hpp"
#include "providers/twitch/TwitchBadge.hpp"

#include <IrcMessage>
#include <QColor>
#include <QUrl>

namespace chatterino {

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

protected:
    virtual void parse();

    virtual void parseUsernameColor();

    virtual void parseUsername();

    // Used for parsing PRIVMSG tags which contain a comma separated list of key-value elements
    std::unordered_map<QString, QString> parseTagList(const QVariantMap &tags,
                                                      const QString &key);

    virtual Outcome tryAppendEmote(const EmoteName &name)
    {
        return Failure;
    }

    // parseHighlights only updates the visual state of the message, but leaves the playing of alerts and sounds to the triggerHighlights function
    virtual void parseHighlights();

    virtual void addTextOrEmoji(EmotePtr emote);
    virtual void addTextOrEmoji(const QString &value);

    void appendChannelName();

    Channel *channel;
    const Communi::IrcMessage *ircMessage;
    MessageParseArgs args;
    const QVariantMap tags;
    QString originalMessage_;

    const bool action_{};

    QColor usernameColor_ = {153, 153, 153};
    MessageColor textColor_ = MessageColor::Text;

    bool highlightAlert_ = false;
    bool highlightSound_ = false;

    QUrl highlightSoundUrl_;
};

}  // namespace chatterino
