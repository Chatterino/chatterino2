#include "IrcChannel2.hpp"

#include "debug/AssertInGuiThread.hpp"
#include "messages/Message.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/irc/IrcCommands.hpp"
#include "providers/irc/IrcServer.hpp"

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

    if (message.startsWith("/"))
    {
        int index = message.indexOf(' ', 1);
        QString command = message.mid(1, index - 1);
        QString params = index == -1 ? "" : message.mid(index + 1);

        invokeIrcCommand(command, params, *this);
    }
    else
    {
        if (this->server())
            this->server()->sendMessage(this->getName(), message);

        MessageBuilder builder;
        builder.emplace<TimestampElement>();
        const auto &nick = this->server()->nick();
        builder.emplace<TextElement>(nick + ":", MessageElementFlag::Username)
            ->setLink({Link::UserInfo, nick});
        builder.emplace<TextElement>(message, MessageElementFlag::Text);
        builder.message().messageText = message;
        builder.message().searchText = nick + ": " + message;
        builder.message().loginName = nick;
        builder.message().displayName = nick;
        this->addMessage(builder.release());
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
        this->server()->connect();
}

}  // namespace chatterino
