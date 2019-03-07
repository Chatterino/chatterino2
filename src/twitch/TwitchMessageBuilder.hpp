#pragma once

#include "messages/Common.hpp"
#include "messages/MessageBuilder.hpp"
#include "util/Outcome.hpp"

#include <IrcMessage>
#include <QString>
#include <QVariant>

namespace chatterino
{
    struct Emote;
    using EmotePtr = std::shared_ptr<const Emote>;

    class Room;
    class TwitchRoom;

    class TwitchMessageBuilder : public MessageBuilder
    {
    public:
        enum UsernameDisplayMode : int {
            Username = 1,                  // Username
            LocalizedName = 2,             // Localized name
            UsernameAndLocalizedName = 3,  // Username (Localized name)
        };

        explicit TwitchMessageBuilder(Room* _channel,
            const Communi::IrcPrivateMessage* _ircMessage,
            const MessageParseArgs& _args);
        explicit TwitchMessageBuilder(Room* _channel,
            const Communi::IrcMessage* _ircMessage,
            const MessageParseArgs& _args, QString content, bool isAction);

        Room* channel;
        TwitchRoom* twitchChannel;
        const Communi::IrcMessage* ircMessage;
        MessageParseArgs args;
        const QVariantMap tags;

        QString messageID;
        QString userName;

        bool isIgnored() const;
        MessagePtr build();

    private:
        void initialize();

        void parseMessageID();
        void parseRoomID();
        void parseUsername();
        void parseHighlights(bool isPastMsg);

        void appendModerationButtons();
        void appendTimestamp();
        void appendChannelName();
        void appendUsername();
        void appendContent();

        // void appendTwitchEmote(const QString& emote,
        //    std::vector<std::tuple<int, EmotePtr, EmoteName>>& vec);
        // Outcome tryAppendEmote(const EmoteName& name);

        void addWords(const QStringList& words,
            const std::vector<std::tuple<int, EmotePtr, EmoteName>>&
                twitchEmotes);
        void addTextOrEmoji(EmotePtr emote);
        void addTextOrEmoji(const QString& value);

        void appendTwitchBadges();
        void appendChatterinoBadges();
        Outcome tryParseCheermote(const QString& string);

        QString roomID_;
        bool hasBits_ = false;

        QColor usernameColor_;
        QString originalMessage_;
        bool senderIsBroadcaster{};

        const bool action_ = false;
        bool isPastMsg_ = false;
    };
}  // namespace chatterino
