#include "IrcChannel2.hpp"

#include "debug/AssertInGuiThread.hpp"
#include "messages/MessageBuilder.hpp"
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

    if (this->server())
        this->server()->sendMessage(this->getName(), message);

    MessageBuilder builder;
    builder.emplace<TimestampElement>();
    builder.emplace<TextElement>(this->server()->nick() + ":",
                                 MessageElementFlag::Username);
    builder.emplace<TextElement>(message, MessageElementFlag::Text);
    this->addMessage(builder.release());
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

}  // namespace chatterino
