#include "IrcServer.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "messages/Message.hpp"
#include "messages/MessageColor.hpp"
#include "messages/MessageElement.hpp"
#include "providers/irc/Irc2.hpp"
#include "providers/irc/IrcChannel2.hpp"
#include "providers/irc/IrcMessageBuilder.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"  // NOTE: Included to access the mentions channel
#include "singletons/Settings.hpp"
#include "util/IrcHelpers.hpp"

#include <QMetaEnum>
#include <QPointer>

#include <cassert>
#include <cstdlib>

namespace chatterino {

IrcServer::IrcServer(const IrcServerData &data)
    : data_(new IrcServerData(data))
{
    this->initializeIrc();

    this->connect();
}

IrcServer::IrcServer(const IrcServerData &data,
                     const std::vector<std::weak_ptr<Channel>> &restoreChannels)
    : IrcServer(data)
{
    for (auto &&weak : restoreChannels)
    {
        if (auto shared = weak.lock())
        {
            this->channels[shared->getName()] = weak;
        }
    }
}

IrcServer::~IrcServer()
{
    delete this->data_;
}

int IrcServer::id()
{
    return this->data_->id;
}

const QString &IrcServer::user()
{
    return this->data_->user;
}

const QString &IrcServer::nick()
{
    return this->data_->nick.isEmpty() ? this->data_->user : this->data_->nick;
}

const QString &IrcServer::userFriendlyIdentifier()
{
    return this->data_->host;
}

void IrcServer::initializeConnectionSignals(IrcConnection *connection,
                                            ConnectionType type)
{
    assert(type == Both);

    QObject::connect(
        connection, &Communi::IrcConnection::socketError, this,
        [this](QAbstractSocket::SocketError error) {
            static int index =
                QAbstractSocket::staticMetaObject.indexOfEnumerator(
                    "SocketError");

            std::lock_guard lock(this->channelMutex);

            for (auto &&weak : this->channels)
            {
                if (auto shared = weak.lock())
                {
                    shared->addMessage(makeSystemMessage(
                        QStringLiteral("Socket error: ") +
                        QAbstractSocket::staticMetaObject.enumerator(index)
                            .valueToKey(error)));
                }
            }
        });

    QObject::connect(connection, &Communi::IrcConnection::nickNameRequired,
                     this, [](const QString &reserved, QString *result) {
                         *result = QString("%1%2").arg(
                             reserved, QString::number(std::rand() % 100));
                     });

    QObject::connect(connection, &Communi::IrcConnection::noticeMessageReceived,
                     this, [this](Communi::IrcNoticeMessage *message) {
                         MessageParseArgs args;
                         args.isReceivedWhisper = true;

                         IrcMessageBuilder builder(message, args);

                         auto msg = builder.build();

                         for (auto &&weak : this->channels)
                         {
                             if (auto shared = weak.lock())
                             {
                                 shared->addMessage(msg);
                             }
                         }
                     });
    QObject::connect(connection,
                     &Communi::IrcConnection::capabilityMessageReceived, this,
                     [this](Communi::IrcCapabilityMessage *message) {
                         const QStringList caps = message->capabilities();
                         if (caps.contains("echo-message"))
                         {
                             this->hasEcho_ = true;
                         }
                     });
}

void IrcServer::initializeConnection(IrcConnection *connection,
                                     ConnectionType type)
{
    assert(type == Both);

    connection->setSecure(this->data_->ssl);
    connection->setHost(this->data_->host);
    connection->setPort(this->data_->port);

    connection->setUserName(this->data_->user);
    connection->setNickName(this->data_->nick.isEmpty() ? this->data_->user
                                                        : this->data_->nick);
    connection->setRealName(this->data_->real.isEmpty() ? this->data_->user
                                                        : this->data_->nick);
    connection->network()->setRequestedCapabilities({"echo-message"});

    if (getSettings()->enableExperimentalIrc)
    {
        switch (this->data_->authType)
        {
            case IrcAuthType::Sasl:
                connection->setSaslMechanism("PLAIN");
                [[fallthrough]];
            case IrcAuthType::Pass:
                this->data_->getPassword(
                    this, [conn = new QPointer(connection) /* can't copy */,
                           this](const QString &password) mutable {
                        if (*conn)
                        {
                            (*conn)->setPassword(password);
                            this->open(Both);
                        }

                        delete conn;
                    });
                break;
            default:
                this->open(Both);
        }
    }
}

std::shared_ptr<Channel> IrcServer::createChannel(const QString &channelName)
{
    return std::make_shared<IrcChannel>(channelName, this);
}

bool IrcServer::hasSeparateWriteConnection() const
{
    return false;
}

void IrcServer::onReadConnected(IrcConnection *connection)
{
    {
        std::lock_guard lock(this->channelMutex);

        for (auto &&command : this->data_->connectCommands)
        {
            connection->sendRaw(command + "\r\n");
        }
    }

    AbstractIrcServer::onReadConnected(connection);
}

void IrcServer::privateMessageReceived(Communi::IrcPrivateMessage *message)
{
    // Note: This doesn't use isPrivate() because it only applies to messages targeting our user,
    // Servers or bouncers may send messages which have our user as the source
    // (like with echo-message CAP), we need to take care of this.
    if (!message->target().startsWith("#"))
    {
        MessageParseArgs args;
        if (message->isOwn())
        {
            // The server sent us a whisper which has our user as the source
            args.isSentWhisper = true;
        }
        else
        {
            args.isReceivedWhisper = true;
        }

        IrcMessageBuilder builder(message, args);

        auto msg = builder.build();

        for (auto &&weak : this->channels)
        {
            if (auto shared = weak.lock())
            {
                shared->addMessage(msg);
            }
        }
        return;
    }

    auto target = message->target();
    target = target.startsWith('#') ? target.mid(1) : target;

    if (auto channel = this->getChannelOrEmpty(target); !channel->isEmpty())
    {
        MessageParseArgs args;
        IrcMessageBuilder builder(channel.get(), message, args);

        if (!builder.isIgnored())
        {
            auto msg = builder.build();

            channel->addMessage(msg);
            builder.triggerHighlights();
            const auto highlighted = msg->flags.has(MessageFlag::Highlighted);
            const auto showInMentions =
                msg->flags.has(MessageFlag::ShowInMentions);

            if (highlighted && showInMentions)
            {
                getIApp()->getTwitch()->getMentionsChannel()->addMessage(msg);
            }
        }
        else
        {
            qCDebug(chatterinoIrc) << "message ignored :rage:";
        }
    }
}

void IrcServer::readConnectionMessageReceived(Communi::IrcMessage *message)
{
    AbstractIrcServer::readConnectionMessageReceived(message);

    switch (message->type())
    {
        case Communi::IrcMessage::Join: {
            auto *x = static_cast<Communi::IrcJoinMessage *>(message);

            if (auto it = this->channels.find(x->channel());
                it != this->channels.end())
            {
                if (auto shared = it->lock())
                {
                    if (message->nick() == this->data_->nick)
                    {
                        shared->addMessage(makeSystemMessage("joined"));
                    }
                    else
                    {
                        if (auto *c =
                                dynamic_cast<ChannelChatters *>(shared.get()))
                        {
                            c->addJoinedUser(x->nick());
                        }
                    }
                }
            }
            return;
        }

        case Communi::IrcMessage::Part: {
            auto *x = static_cast<Communi::IrcPartMessage *>(message);

            if (auto it = this->channels.find(x->channel());
                it != this->channels.end())
            {
                if (auto shared = it->lock())
                {
                    if (message->nick() == this->data_->nick)
                    {
                        shared->addMessage(makeSystemMessage("parted"));
                    }
                    else
                    {
                        if (auto *c =
                                dynamic_cast<ChannelChatters *>(shared.get()))
                        {
                            c->addPartedUser(x->nick());
                        }
                    }
                }
            }
            return;
        }

        case Communi::IrcMessage::Pong:
        case Communi::IrcMessage::Notice:
        case Communi::IrcMessage::Private:
            return;

        default:
            if (getSettings()->showUnhandledIrcMessages)
            {
                MessageBuilder builder;

                builder.emplace<TimestampElement>(
                    calculateMessageTime(message).time());
                builder.emplace<TextElement>(message->toData(),
                                             MessageElementFlag::Text);
                builder->flags.set(MessageFlag::Debug);

                auto msg = builder.release();

                for (auto &&weak : this->channels)
                {
                    if (auto shared = weak.lock())
                    {
                        shared->addMessage(msg);
                    }
                }
            };
    }
}

void IrcServer::sendWhisper(const QString &target, const QString &message)
{
    this->sendRawMessage(QString("PRIVMSG %1 :%2").arg(target, message));
    if (this->hasEcho())
    {
        return;
    }

    MessageParseArgs args;
    args.isSentWhisper = true;

    MessageBuilder b;

    b.emplace<TimestampElement>();
    b.emplace<TextElement>(this->nick(), MessageElementFlag::Text,
                           MessageColor::Text, FontStyle::ChatMediumBold);
    b.emplace<TextElement>("->", MessageElementFlag::Text,
                           MessageColor::System);
    b.emplace<TextElement>(target + ":", MessageElementFlag::Text,
                           MessageColor::Text, FontStyle::ChatMediumBold);
    b.emplace<TextElement>(message, MessageElementFlag::Text);

    auto msg = b.release();
    for (auto &&weak : this->channels)
    {
        if (auto shared = weak.lock())
        {
            shared->addMessage(msg);
        }
    }
}

void IrcServer::sendRawMessage(const QString &rawMessage)
{
    AbstractIrcServer::sendRawMessage(rawMessage.left(510));
}

bool IrcServer::hasEcho() const
{
    return this->hasEcho_;
}

}  // namespace chatterino
