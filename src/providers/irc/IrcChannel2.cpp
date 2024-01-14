#include "IrcChannel2.hpp"

#include "debug/AssertInGuiThread.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "messages/MessageElement.hpp"
#include "providers/irc/IrcCommands.hpp"
#include "providers/irc/IrcMessageBuilder.hpp"
#include "providers/irc/IrcServer.hpp"
#include "util/Helpers.hpp"

namespace chatterino {

IrcChannel::IrcChannel(const QString &name, IrcServer *server)
    : Channel(name, Channel::Type::Irc)
    , ChannelChatters(*static_cast<Channel *>(this))
    , server_(server)
{
}

void IrcChannel::sendMessage(const QString &message)
{
    assertInGuiThread();
    if (message.isEmpty())
    {
        return;
    }

    if (message.startsWith("/"))
    {
        int index = message.indexOf(' ', 1);
        QString command = message.mid(1, index - 1);
        QString params = index == -1 ? "" : message.mid(index + 1);

        invokeIrcCommand(command, params, *this);
    }
    else
    {
        if (this->server() != nullptr)
        {
            this->server()->sendMessage(this->getName(), message);
            if (this->server()->hasEcho())
            {
                return;
            }
            MessageBuilder builder;

            builder
                .emplace<TextElement>("#" + this->getName(),
                                      MessageElementFlag::ChannelName,
                                      MessageColor::System)
                ->setLink({Link::JumpToChannel, this->getName()});

            auto now = QDateTime::currentDateTime();
            builder.emplace<TimestampElement>(now.time());
            builder.message().serverReceivedTime = now;

            auto username = this->server()->nick();
            builder
                .emplace<TextElement>(
                    username + ":", MessageElementFlag::Username,
                    getRandomColor(username), FontStyle::ChatMediumBold)
                ->setLink({Link::UserInfo, username});
            builder.message().loginName = username;
            builder.message().displayName = username;

            // message
            builder.addIrcMessageText(message);
            builder.message().messageText = message;
            builder.message().searchText = username + ": " + message;

            this->addMessage(builder.release());
        }
        else
        {
            this->addMessage(makeSystemMessage("You are not connected."));
        }
    }
}

IrcServer *IrcChannel::server()
{
    assertInGuiThread();

    return this->server_;
}

void IrcChannel::setServer(IrcServer *server)
{
    assertInGuiThread();

    this->server_ = server;
}

bool IrcChannel::canReconnect() const
{
    return true;
}

void IrcChannel::reconnect()
{
    if (this->server())
    {
        this->server()->connect();
    }
}

}  // namespace chatterino
