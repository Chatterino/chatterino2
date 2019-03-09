#include "TwitchHandleMessage.hpp"

#include "Room.hpp"
#include "messages/Message.hpp"
#include "messages/MessageContainer.hpp"
#include "twitch/TwitchMessageBuilder.hpp"
#include "twitch/TwitchRoom.hpp"

#include <unordered_set>

namespace chatterino
{
    namespace
    {
        void addMessage(Communi::IrcMessage* _message, const QString& target,
            const QString& content, TwitchRoom& room, bool isSub, bool isAction)
        {
            MessageParseArgs args;

            if (isSub)
            {
                args.trimSubscriberUsername = true;
            }

            // if (chan->isBroadcaster())
            //{
            //    args.isStaffOrBroadcaster = true;
            //}

            TwitchMessageBuilder builder(
                &room, _message, args, content, isAction);

            if (isSub || !builder.isIgnored())
            {
                if (isSub)
                {
                    builder->flags.set(MessageFlag::Subscription);
                    builder->flags.unset(MessageFlag::Highlighted);
                }

                auto msg = builder.build();
                auto highlighted = msg->flags.has(MessageFlag::Highlighted);

                if (!isSub)
                {
                    if (highlighted)
                    {
                        // server.mentionsChannel->addMessage(msg);
                        // getApp()->highlights->addHighlight(msg);
                    }
                }

                room.messages().append(msg);
            }
        }

        void roomState(Communi::IrcMessage* message, TwitchRoom& room)
        {
            const auto& tags = message->tags();

            // Room id
            decltype(tags.find("")) it;

            if ((it = tags.find("room-id")) != tags.end())
            {
                auto roomId = it.value().toString();

                // TODO: check if this is actually the user id or some other
                // type of id
                room.setUserId(roomId);
            }

            // Room modes
            auto roomModes = TwitchRoomModes();

            if ((it = tags.find("emote-only")) != tags.end())
            {
                roomModes.emoteOnly = it.value() == "1";
            }
            if ((it = tags.find("subs-only")) != tags.end())
            {
                roomModes.subOnly = it.value() == "1";
            }
            if ((it = tags.find("slow")) != tags.end())
            {
                roomModes.slowMode = it.value().toInt();
            }
            if ((it = tags.find("r9k")) != tags.end())
            {
                roomModes.r9k = it.value() == "1";
            }
            if ((it = tags.find("broadcaster-lang")) != tags.end())
            {
                roomModes.broadcasterLanguage = it.value().toString();
            }

            room.setModes(roomModes);
        }

        void clearChat(Communi::IrcMessage* message, TwitchRoom& room)
        {
            // check parameter count
            if (message->parameters().length() < 1)
            {
                return;
            }

            // check if the chat has been cleared by a moderator
            if (message->parameters().length() == 1)
            {
                // chan->disableAllMessages();
                // chan->addMessage(
                //    makeSystemMessage("Chat has been cleared by a
                //    moderator."));

                return;
            }

            // get username, duration and message of the timed out user
            QString username = message->parameter(1);
            QString durationInSeconds = message->tag("ban-duration").toString();
            QString reason = message->tag("ban-reason").toString();

            // auto timeoutMsg = MessageBuilder(
            //     timeoutMessage, username, durationInSeconds, reason, false)
            //                       .release();
            // chan->addOrReplaceTimeout(timeoutMsg);

            // refresh all
            room.repaint();
        }

        void userState(Communi::IrcMessage* message, TwitchRoom& room)
        {
            auto mod = message->tag("mod");
            if (mod.isValid())
            {
                room.setMod(mod == "1");
            }
        }

        void whisper(Communi::IrcMessage* message)
        {
            // MessageParseArgs args;

            // args.isReceivedWhisper = true;

            // auto c = app->twitch.server->whispersChannel.get();

            // TwitchMessageBuilder builder(
            // c, message, args, message->parameter(1), false);

            // if (!builder.isIgnored())
            //{
            // MessagePtr _message = builder.build();

            // app->twitch.server->lastUserThatWhisperedMe.set(
            // builder.userName);

            // if (_message->flags.has(MessageFlag::Highlighted))
            //{
            // app->twitch.server->mentionsChannel->addMessage(_message);
            //}

            // c->addMessage(_message);

            // auto overrideFlags =
            // boost::optional<MessageFlags>(_message->flags);
            // overrideFlags->set(MessageFlag::DoNotTriggerNotification);

            // if (getSettings()->inlineWhispers)
            //{
            // app->twitch.server->forEachChannel(
            //[_message, overrideFlags](ChannelPtr channel) {
            //	channel->addMessage(_message, overrideFlags);  //
            //});
            //}
            //}
        }

        void userNotice(Communi::IrcMessage* message, TwitchRoom& room)
        {
            auto data = message->toData();

            auto tags = message->tags();
            auto parameters = message->parameters();

            auto target = parameters[0];
            QString msgType = tags.value("msg-id", "").toString();
            QString content;
            if (parameters.size() >= 2)
            {
                content = parameters[1];
            }

            if (msgType == "sub" || msgType == "resub" || msgType == "subgift")
            {
                // Sub-specific message. I think it's only allowed for "resub"
                // messages atm
                if (!content.isEmpty())
                {
                    addMessage(message, target, content, room, true, false);
                }
            }

            auto it = tags.find("system-msg");

            if (it != tags.end())
            {
                // auto b = MessageBuilder(
                // systemMessage, parseTagString(it.value().toString()));

                // b->flags.set(MessageFlag::Subscription);
                // auto newMessage = b.release();

                // room.addMessage(newMessage);
            }
        }

        void mode(Communi::IrcMessage* message, TwitchRoom& room)
        {
            // auto app = getApp();

            // auto channel = app->twitch.server->getChannelOrEmpty(
            // message->parameter(0).remove(0, 1));

            // if (channel->isEmpty())
            //{
            // return;
            //}

            // if (message->parameter(1) == "+o")
            //{
            // channel->modList.append(message->parameter(2));
            //}
            // else if (message->parameter(1) == "-o")
            //{
            // channel->modList.append(message->parameter(2));
            //}
        }

        void notice(Communi::IrcMessage* message_, TwitchRoom& room)
        {
            auto message = static_cast<Communi::IrcNoticeMessage*>(message_);

            // auto msg = makeSystemMessage(message->content());

            // QString channelName;
            // if (!trimChannelName(message->target(), channelName))
            //{
            //    // Notice wasn't targeted at a single channel, send to all
            //    // twitch channels
            //    app->twitch.server->forEachChannelAndSpecialChannels(
            //        [msg](const auto& c) {
            //            c->addMessage(msg);  //
            //        });

            //    return;
            //}

            // auto channel =
            // app->twitch.server->getChannelOrEmpty(channelName);

            // if (channel->isEmpty())
            //{
            //    log("[IrcManager:handleNoticeMessage] Channel {} not found in
            //    "
            //        "channel "
            //        "manager ",
            //        channelName);
            //    return;
            //}

            // channel->addMessage(msg);
        }

        void writeConnectionNotice(
            Communi::IrcNoticeMessage* message, TwitchRoom& room)
        {
            static std::unordered_set<std::string> readConnectionOnlyIDs{
                "host_on",
                "host_off",
                "host_target_went_offline",
                "emote_only_on",
                "emote_only_off",
                "slow_on",
                "slow_off",
                "subs_on",
                "subs_off",
                "r9k_on",
                "r9k_off",

                // Display for user who times someone out. This implies you're a
                // moderator, at which point you will be connected to PubSub and
                // receive a better message from there
                "timeout_success",
                "ban_success",

                // Channel suspended notices
                "msg_channel_suspended",
            };

            QVariant v = message->tag("msg-id");
            if (v.isValid())
            {
                std::string msgID = v.toString().toStdString();

                if (readConnectionOnlyIDs.find(msgID) !=
                    readConnectionOnlyIDs.end())
                {
                    return;
                }

                //                log("Showing notice message from write
                //                connection with message "
                //                    "id "
                //                    "'{}'",
                //                    msgID);
            }

            // handleNoticeMessage(message, room);
        }

        void join(Communi::IrcMessage* message, TwitchRoom& room)
        {
            room.addChatter(message->nick());
        }

        void part(Communi::IrcMessage* message, TwitchRoom& room)
        {
            room.removeChatter(message->nick());
        }
    }  // namespace

    void handleMessage(Communi::IrcMessage* message, TwitchRoom* room)
    {
        if (message->type() == Communi::IrcMessage::Type::Private)
        {
            // We already have a handler for private messages
            return;
        }

        const QString& command = message->command();

        // switch through all the commands and set roomFunc or noRoomFunc
        // accordingly
        void (*roomFunc)(Communi::IrcMessage*, TwitchRoom&) = nullptr;
        void (*noRoomFunc)(Communi::IrcMessage*) = nullptr;

        if (command == "ROOMSTATE")
            roomFunc = roomState;
        else if (command == "CLEARCHAT")
            roomFunc = clearChat;
        else if (command == "USERSTATE")
            roomFunc = userState;
        else if (command == "WHISPER")
            noRoomFunc = whisper;
        else if (command == "USERNOTICE")
            roomFunc = userNotice;
        else if (command == "MODE")
            roomFunc = mode;
        else if (command == "NOTICE")
            roomFunc = notice;
        else if (command == "JOIN")
            roomFunc = join;
        else if (command == "PART")
            roomFunc = part;

        // execute the function that was selected
        if (noRoomFunc)
            noRoomFunc(message);

        // room is required to be a valid object for this function
        if (roomFunc && room)
            roomFunc(message, *room);
    }

    void handlePrivMsg(Communi::IrcPrivateMessage* message, TwitchRoom& room)
    {
        addMessage(message, message->target(), message->content(), room, false,
            message->isAction());
    }
}  // namespace chatterino
