#include "Irc2.hpp"

#include <pajlada/serialize.hpp>
#include "common/Credentials.hpp"
#include "common/SignalVectorModel.hpp"
#include "singletons/Paths.hpp"
#include "util/CombinePath.hpp"
#include "util/RapidjsonHelpers.hpp"
#include "util/StandardItemHelper.hpp"

#include <QSaveFile>
#include <QtConcurrent>

namespace chatterino {

namespace {
    QString configPath()
    {
        return combinePath(getPaths()->settingsDirectory, "irc.json");
    }

    class Model : public SignalVectorModel<IrcServerData>
    {
    public:
        Model(QObject *parent)
            : SignalVectorModel<IrcServerData>(6, parent)
        {
        }

        // turn a vector item into a model row
        IrcServerData getItemFromRow(std::vector<QStandardItem *> &row,
                                     const IrcServerData &original)
        {
            return IrcServerData{
                row[0]->data(Qt::EditRole).toString(),      // host
                row[1]->data(Qt::EditRole).toInt(),         // port
                row[2]->data(Qt::CheckStateRole).toBool(),  // ssl
                row[3]->data(Qt::EditRole).toString(),      // user
                row[4]->data(Qt::EditRole).toString(),      // nick
                row[5]->data(Qt::EditRole).toString(),      // real
                original.authType,                          // authType
                original.connectCommands,                   // connectCommands
                original.id,                                // id
            };
        }

        // turns a row in the model into a vector item
        void getRowFromItem(const IrcServerData &item,
                            std::vector<QStandardItem *> &row)
        {
            setStringItem(row[0], item.host, false);
            setStringItem(row[1], QString::number(item.port));
            setBoolItem(row[2], item.ssl);
            setStringItem(row[3], item.user);
            setStringItem(row[4], item.nick);
            setStringItem(row[5], item.real);
        }
    };
}  // namespace

inline QString escape(QString str)
{
    return str.replace(":", "::");
}

// This returns a unique id for every server which is understandeable in the systems credential manager.
inline QString getCredentialName(const IrcServerData &data)
{
    return escape(QString::number(data.id)) + ":" + escape(data.user) + "@" +
           escape(data.host);
}

void IrcServerData::getPassword(
    QObject *receiver, std::function<void(const QString &)> &&onLoaded) const
{
    Credentials::instance().get("irc", getCredentialName(*this), receiver,
                                std::move(onLoaded));
}

void IrcServerData::setPassword(const QString &password)
{
    Credentials::instance().set("irc", getCredentialName(*this), password);
}

Irc::Irc()
{
    this->connections.itemInserted.connect([this](auto &&args) {
        // make sure only one id can only exist for one server
        assert(this->servers_.find(args.item.id) == this->servers_.end());

        // add new server
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

        if (args.caller != Irc::noEraseCredentialCaller)
        {
            Credentials::instance().erase("irc", getCredentialName(args.item));
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
        auto channel = std::make_shared<IrcChannel>(name, nullptr);

        this->abandonedChannels_[id].push_back(channel);

        return std::move(channel);
    }
}

Irc &Irc::instance()
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
        obj.insert("connectCommands",
                   QJsonArray::fromStringList(conn.connectCommands));
        obj.insert("id", conn.id);
        obj.insert("authType", int(conn.authType));

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
    auto object = QJsonDocument::fromJson(file.readAll()).object();

    std::unordered_set<int> ids;

    // load servers
    for (auto server : object.value("servers").toArray())
    {
        auto obj = server.toObject();
        IrcServerData data;
        data.host = obj.value("host").toString(data.host);
        data.port = obj.value("port").toInt(data.port);
        data.ssl = obj.value("ssl").toBool(data.ssl);
        data.user = obj.value("username").toString(data.user);
        data.nick = obj.value("nickname").toString(data.nick);
        data.real = obj.value("realname").toString(data.real);
        data.connectCommands =
            obj.value("connectCommands").toVariant().toStringList();
        data.id = obj.value("id").toInt(data.id);
        data.authType =
            IrcAuthType(obj.value("authType").toInt(int(data.authType)));

        // duplicate id's are not allowed :(
        if (ids.find(data.id) == ids.end())
        {
            ids.insert(data.id);

            this->connections.appendItem(data);
        }
    }
}

}  // namespace chatterino
