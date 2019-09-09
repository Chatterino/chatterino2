#include "Irc2.hpp"

#include <pajlada/serialize.hpp>
#include "common/Credentials.hpp"
#include "common/SignalVectorModel.hpp"
#include "util/RapidjsonHelpers.hpp"
#include "util/StandardItemHelper.hpp"

namespace chatterino {

namespace {
    class Model : public SignalVectorModel<IrcConnection_>
    {
    public:
        Model(QObject *parent)
            : SignalVectorModel<IrcConnection_>(6, parent)
        {
        }

        // turn a vector item into a model row
        IrcConnection_ getItemFromRow(std::vector<QStandardItem *> &row,
                                      const IrcConnection_ &original)
        {
            return IrcConnection_{
                row[0]->data(Qt::EditRole).toString(),  // host
                row[1]->data(Qt::EditRole).toInt(),     // port
                row[2]->data(Qt::Checked).toBool(),     // ssl
                row[3]->data(Qt::EditRole).toString(),  // user
                row[4]->data(Qt::EditRole).toString(),  // nick
                row[5]->data(Qt::EditRole).toString(),  // password
                original.id,                            // id
            };
        }

        // turns a row in the model into a vector item
        void getRowFromItem(const IrcConnection_ &item,
                            std::vector<QStandardItem *> &row)
        {
            setStringItem(row[0], item.host);
            setStringItem(row[1], QString::number(item.port));
            setBoolItem(row[2], item.ssl);
            setStringItem(row[3], item.user);
            setStringItem(row[4], item.nick);
            setStringItem(row[5], item.password);
        }
    };
}  // namespace

static std::atomic_int currentId;

//inline QString escape(QString str)
//{
//    return str.replace(":", "::");
//}

//inline QString getCredentialName(const IrcConnection_ &conn)
//{
//    //return escape(conn.host) + escape(conn.
//}

//QString IrcConnection_::getPassword()
//{
//    return
//}

//void IrcConnection_::setPassword(const QString &str)
//{
//    //Credentials::set("irc",
//}

Irc::Irc()
{
    this->connections.itemInserted.connect([this](auto &&args) {
        // make sure only one id can only exist for one server
        assert(this->servers_.find(args.item.id) == this->servers_.end());

        if (auto ab = this->abandonedChannels_.find(args.item.id);
            ab != this->abandonedChannels_.end())
        {
            // add new server with abandoned channels
            this->servers_.emplace(args.item.id, std::make_unique<IrcServer>(
                                                     args.item, ab->second));
            this->abandonedChannels_.erase(ab);
        }
        else
        {
            // add new server
            this->servers_.emplace(args.item.id,
                                   std::make_unique<IrcServer>(args.item));
        }
    });

    this->connections.itemRemoved.connect([this](auto &&args) {
        // restore
        if (auto server = this->servers_.find(args.item.id);
            server != this->servers_.end())
        {
            this->abandonedChannels_[args.item.id] =
                server->second->getChannels();
            this->servers_.erase(server);
        }
    });
}

IrcConnection_ IrcConnection_::unique()
{
    IrcConnection_ c;
    c.host = "localhost";
    c.id = currentId++;
    c.port = 6697;
    c.user = "xD";
    c.nick = "xD";
    return c;
}

QAbstractTableModel *Irc::newConnectionModel(QObject *parent)
{
    auto model = new Model(parent);
    model->init(&this->connections);
    return model;
}

IrcServer *Irc::getServerOfChannel(Channel *channel)
{
    for (auto &&server : this->servers_)
        for (auto weak : server.second->getChannels())
            if (auto shared = weak.lock())
                if (shared.get() == channel)
                    return server.second.get();

    return nullptr;
}

ChannelPtr Irc::getOrAddChannel(int id, QString name)
{
    if (auto server = this->servers_.find(id); server != this->servers_.end())
    {
        return server->second->getOrAddChannel(name);
    }
    else
    {
        return Channel::getEmpty();
    }
}

Irc &Irc::getInstance()
{
    static Irc irc;
    return irc;
}

}  // namespace chatterino
