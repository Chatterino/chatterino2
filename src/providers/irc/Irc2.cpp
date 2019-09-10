#include "Irc2.hpp"

#include <pajlada/serialize.hpp>
#include "common/Credentials.hpp"
#include "common/SignalVectorModel.hpp"
#include "singletons/Paths.hpp"
#include "util/CombinePath.hpp"
#include "util/RapidjsonHelpers.hpp"
#include "util/StandardItemHelper.hpp"

#include <QSaveFile>

namespace chatterino {

namespace {
    QString configPath()
    {
        return combinePath(getPaths()->settingsDirectory, "irc.json");
    }

    class Model : public SignalVectorModel<IrcConnection_>
    {
    public:
        Model(QObject *parent)
            : SignalVectorModel<IrcConnection_>(8, parent)
        {
        }

        // turn a vector item into a model row
        IrcConnection_ getItemFromRow(std::vector<QStandardItem *> &row,
                                      const IrcConnection_ &original)
        {
            qDebug() << row[2]->data(Qt::CheckStateRole).toBool();

            return IrcConnection_{
                row[0]->data(Qt::EditRole).toString(),      // host
                row[1]->data(Qt::EditRole).toInt(),         // port
                row[2]->data(Qt::CheckStateRole).toBool(),  // ssl
                row[3]->data(Qt::EditRole).toString(),      // user
                row[4]->data(Qt::EditRole).toString(),      // nick
                row[5]->data(Qt::EditRole).toString(),      // real
                row[6]->data(Qt::EditRole).toString(),      // password
                original.id,                                // id
            };
        }

        // turns a row in the model into a vector item
        void getRowFromItem(const IrcConnection_ &item,
                            std::vector<QStandardItem *> &row)
        {
            setStringItem(row[0], item.host, false);
            setStringItem(row[1], QString::number(item.port));
            setBoolItem(row[2], item.ssl);
            setStringItem(row[3], item.user);
            setStringItem(row[4], item.nick);
            setStringItem(row[5], item.real);
            setStringItem(row[6], item.password);
        }
    };
}  // namespace

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
            auto server = std::make_unique<IrcServer>(args.item, ab->second);

            // set server of abandoned channels
            for (auto weak : ab->second)
                if (auto shared = weak.lock())
                    if (auto ircChannel =
                            dynamic_cast<IrcChannel *>(shared.get()))
                        ircChannel->setServer(server.get());

            // add new server with abandoned channels
            this->servers_.emplace(args.item.id, std::move(server));
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
            auto abandoned = server->second->getChannels();

            // set server of abandoned servers to nullptr
            for (auto weak : abandoned)
                if (auto shared = weak.lock())
                    if (auto ircChannel =
                            dynamic_cast<IrcChannel *>(shared.get()))
                        ircChannel->setServer(nullptr);

            this->abandonedChannels_[args.item.id] = abandoned;
            this->servers_.erase(server);
        }
    });

    this->connections.delayedItemsChanged.connect([this] { this->save(); });
}

QAbstractTableModel *Irc::newConnectionModel(QObject *parent)
{
    auto model = new Model(parent);
    model->init(&this->connections);
    return model;
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

int Irc::uniqueId()
{
    int i = this->currentId_ + 1;
    auto it = this->servers_.find(i);
    auto it2 = this->abandonedChannels_.find(i);

    while (it != this->servers_.end() || it2 != this->abandonedChannels_.end())
    {
        i++;
        it = this->servers_.find(i);
        it2 = this->abandonedChannels_.find(i);
    }

    return (this->currentId_ = i);
}

void Irc::save()
{
    QJsonDocument doc;
    QJsonObject root;
    QJsonArray servers;

    for (auto &&conn : this->connections)
    {
        QJsonObject obj;
        obj.insert("host", conn.host);
        obj.insert("port", conn.port);
        obj.insert("ssl", conn.ssl);
        obj.insert("username", conn.user);
        obj.insert("nickname", conn.nick);
        obj.insert("realname", conn.real);
        //obj.insert("password", conn.password);
        obj.insert("id", conn.id);
        servers.append(obj);
    }

    root.insert("servers", servers);
    doc.setObject(root);

    QSaveFile file(configPath());
    file.open(QIODevice::WriteOnly);
    file.write(doc.toJson());
    file.commit();
}

void Irc::load()
{
    if (this->loaded_)
        return;
    this->loaded_ = true;

    QString config = configPath();
    QFile file(configPath());
    file.open(QIODevice::ReadOnly);
    auto doc = QJsonDocument::fromJson(file.readAll());

    auto object = doc.object();
    std::unordered_set<int> ids;

    for (auto server : doc.object().value("servers").toArray())
    {
        auto obj = server.toObject();
        IrcConnection_ conn;
        conn.host = obj.value("host").toString(conn.host);
        conn.port = obj.value("port").toInt(conn.port);
        conn.ssl = obj.value("ssl").toBool(conn.ssl);
        conn.user = obj.value("username").toString(conn.user);
        conn.nick = obj.value("nickname").toString(conn.nick);
        conn.real = obj.value("realname").toString(conn.real);
        // conn.password = obj.value("password").toString(conn.password);
        conn.id = obj.value("id").toInt(conn.id);

        // duplicate id's are not allowed :(
        if (ids.find(conn.id) == ids.end())
        {
            this->connections.appendItem(conn);
            ids.insert(conn.id);
        }
    }
}

}  // namespace chatterino
