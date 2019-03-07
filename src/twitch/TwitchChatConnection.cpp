#include "twitch/TwitchChatConnection.hpp"

#include <QHostAddress>
#include <irc/IrcConnection2.hpp>

namespace chatterino
{
    namespace
    {
        class Private
        {
        public:
            IrcConnection* read{};   // set parent
            IrcConnection* write{};  // set parent

            QSet<QString> channels;
        };
    }  // namespace

    TwitchChatConnection::TwitchChatConnection(QObject* parent)
        : QObject(parent)
        , this_(new Private)
    {
        // read
        this_->read = new IrcConnection(parent);
        this_->read->setHost("irc.chat.twitch.tv");
        this_->read->setPort(443);
        this_->read->setSecure(true);
        this_->read->setUserName("justinfan123");
        this_->read->setNickName("justinfan123");
        this_->read->setRealName("justinfan123");

        // connect to connected signal
        QObject::connect(
            this_->read, &IrcConnection::connected, this, [this]() {
                this_->read->sendRaw("CAP REQ :twitch.tv/commands");
                this_->read->sendRaw("CAP REQ :twitch.tv/tags");

                for (auto&& channel : this_->channels)
                    this_->read->sendRaw("JOIN #" + channel);
            });

        // write
        this_->write = new IrcConnection(parent);

        // connect to connected signal
        QObject::connect(
            this_->read, &IrcConnection::connected, this, [this]() {
                for (auto&& channel : this_->channels)
                    this_->write->sendRaw("JOIN #" + channel);
            });
    }

    TwitchChatConnection::~TwitchChatConnection()
    {
        delete this_;
    }

    void TwitchChatConnection::connect()
    {
        if (this_->read)
            this_->read->open();
    }

    void TwitchChatConnection::join(const QString& name)
    {
        // add to channels list
        assert(!this_->channels.contains(name));
        this_->channels.insert(name);

        // send join command to read and write connection
        if (this_->read && this_->read->isConnected())
            this_->read->sendRaw("JOIN #" + name);

        if (this_->write && this_->write->isConnected())
            this_->write->sendRaw("JOIN #" + name);
    }

    void TwitchChatConnection::part(const QString& name)
    {
        // remove from channels list
        assert(this_->channels.contains(name));
        this_->channels.remove(name);

        // send part command to read and write connection
        if (this_->read)
            this_->read->sendRaw("PART #" + name);

        if (this_->write)
            this_->write->sendRaw("PART #" + name);
    }

    IrcConnection* TwitchChatConnection::getReadConnection()
    {
        return this_->read;
    }

    IrcConnection* TwitchChatConnection::getWriteConnection()
    {
        return this_->write;
    }
}  // namespace chatterino
