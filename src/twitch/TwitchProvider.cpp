#include "TwitchProvider.hpp"

#include <QDebug>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QString>
#include <unordered_map>

#include "ab/Column.hpp"
#include "ab/util/FunctionEventFilter.hpp"
#include "ab/util/MakeWidget.hpp"

#include "irc/IrcConnection2.hpp"
#include "twitch/TwitchChatConnection.hpp"
#include "twitch/TwitchHandleMessage.hpp"
#include "twitch/TwitchRoom.hpp"
#include "util/QStringHash.hpp"

#include "twitch/TwitchBadges.hpp"

namespace chatterino
{
    namespace
    {
        QString getChannelName(Communi::IrcMessage* message)
        {
            auto name = message->parameter(0);

            if (name[0] == '#')
                return name.mid(1);
            else
                return QString();
        }
    }  // namespace

    class TwitchProviderData
    {
    public:
        std::unique_ptr<TwitchChatConnection> connection;
        std::unique_ptr<QObject> object;

        std::unordered_map<QString, std::weak_ptr<TwitchRoom>> rooms;

        RoomPtr mentions;
        RoomPtr whispers;
    };

    TwitchProvider::TwitchProvider()
        : this_(new TwitchProviderData)
    {
        this_->object = std::make_unique<QObject>();

        // default channels
        this_->mentions = std::make_shared<TwitchMentionsRoom>();
        this_->whispers = std::make_shared<TwitchWhispersRoom>();

        // twitch chat connection
        this_->connection = std::make_unique<TwitchChatConnection>();
        this_->connection->connect();

        // message received
        QObject::connect(this_->connection->getReadConnection(),
            &IrcConnection::messageReceived, this_->object.get(),
            [=](auto message) {
                qDebug() << QString(message->toData());

                // search for the room
                auto room = this_->rooms.find(getChannelName(message));
                if (room != this_->rooms.end())
                {
                    // get shared ptr from the weak ptr
                    auto roomPtr = room->second.lock();
                    handleMessage(message, roomPtr.get());
                }
            });

        // privmsg received
        QObject::connect(this_->connection->getReadConnection(),
            &IrcConnection::privateMessageReceived, this_->object.get(),
            [=](Communi::IrcPrivateMessage* message) {
                // search for the room
                auto room = this_->rooms.find(getChannelName(message));
                if (room != this_->rooms.end())
                    // try to get a shared ptr from the weak ptr
                    if (auto roomPtr = room->second.lock())
                        handlePrivMsg(message, *roomPtr);
            });

        // init stuff
        twitchBadges().load();
    }

    TwitchProvider::~TwitchProvider()
    {
        delete this_;
    }

    const QString& TwitchProvider::name()
    {
        static QString name = "twitch";

        return name;
    }

    const QString& TwitchProvider::title()
    {
        static QString name = "Twitch";

        return name;
    }

    std::pair<QLayout*, std::function<RoomPtr()>>
        TwitchProvider::buildSelectChannelLayout(const QJsonObject& data)
    {
        // parse json values
        auto type = data["type"].toString();
        auto channel = data["name"].toString();

        // create widgets
        auto isName = ab::makeWidget<QRadioButton>(
            [&](auto x) { x->setText("Join channel by name"); });
        auto name =
            ab::makeWidget<QLineEdit>([&](auto x) { x->setText(channel); });

        auto isWhispers = ab::makeWidget<QRadioButton>(
            [](auto x) { x->setText("Whispers"); });
        auto isMentions = ab::makeWidget<QRadioButton>(
            [](auto x) { x->setText("Mentions"); });

        // set radio checkedness
        if (type == "whispers")
            isWhispers->setChecked(true);
        else if (type == "mentions")
            isMentions->setChecked(true);
        else
            isName->setChecked(true);

        return {
            ab::makeLayout<ab::Column>({
                isName,
                name,
                isMentions,
                isWhispers,
                ab::stretch(),
            }),
            [=]() -> RoomPtr {
                return this->addRoom([&]() -> QJsonObject {
                    if (isName->isChecked())
                        return {{"name", name->text()}};
                    else if (isWhispers->isChecked())
                        return {{"type", "whispers"}};
                    else if (isMentions->isChecked())
                        return {{"type", "mentions"}};
                    else
                    {
                        assert(false);
                        return {{}};
                    }
                }());
            },
        };
    }

    RoomPtr TwitchProvider::addRoom(const QJsonObject& data)
    {
        auto type = data["type"].toString();
        auto channel = data["name"].toString();

        if (type == "whispers")
            return this_->whispers;
        else if (type == "mentions")
            return this_->mentions;
        else if (!channel.isNull())
        {
            auto& weakRoom = this_->rooms[channel];
            auto room = weakRoom.lock();

            // create room if it doesn't exist yet
            if (!room)
            {
                weakRoom = room = std::make_shared<TwitchRoom>(channel);

                // join irc channel
                this_->connection->join(channel);

                // remove room from map when it gets destroyed
                QObject::connect(room.get(), &TwitchRoom::destroyed,
                    // remove room
                    this_->object.get(), [this, channel]() {
                        this_->rooms.erase(this_->rooms.find(channel));
                    });
            }

            return RoomPtr(room);
        }
        else
            return emptyRoom();
    }
}  // namespace chatterino
