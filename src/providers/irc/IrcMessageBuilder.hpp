#pragma once

#include "common/Aliases.hpp"
#include "common/Outcome.hpp"
#include "messages/SharedMessageBuilder.hpp"
#include "providers/twitch/TwitchBadge.hpp"

#include <IrcMessage>
#include <QString>
#include <QVariant>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class Channel;
class TwitchChannel;

class IrcMessageBuilder : public SharedMessageBuilder
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

    /**
     * @brief used for global notice messages (i.e. notice messages without a channel as its target)
     **/
    explicit IrcMessageBuilder(const Communi::IrcNoticeMessage *_ircMessage,
                               const MessageParseArgs &_args);

    /**
     * @brief used for whisper messages (i.e. PRIVMSG messages with our nick as the target)
     **/
    explicit IrcMessageBuilder(const Communi::IrcPrivateMessage *_ircMessage,
                               const MessageParseArgs &_args);

    MessagePtr build() override;

private:
    void appendUsername();

    /**
     * @brief holds the name of the target for the private/direct IRC message
     *
     * This might not be our nick
     */
    QString whisperTarget_;
};

}  // namespace chatterino
