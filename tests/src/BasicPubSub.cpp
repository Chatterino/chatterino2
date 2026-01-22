// SPDX-FileCopyrightText: 2022 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "mocks/BaseApplication.hpp"
#include "providers/liveupdates/BasicPubSubClient.hpp"
#include "providers/liveupdates/BasicPubSubManager.hpp"
#include "Test.hpp"

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QtCore/qtestsupport_core.h>

#include <deque>
#include <mutex>
#include <optional>

using namespace chatterino;
using namespace std::chrono_literals;

namespace {

struct DummySubscription {
    int type;
    QString condition;

    bool operator==(const DummySubscription &rhs) const
    {
        return std::tie(this->condition, this->type) ==
               std::tie(rhs.condition, rhs.type);
    }
    bool operator!=(const DummySubscription &rhs) const
    {
        return !(rhs == *this);
    }

    QByteArray encodeSubscribe() const
    {
        QJsonObject root;
        root["op"] = "sub";
        root["type"] = this->type;
        root["condition"] = this->condition;
        return QJsonDocument(root).toJson();
    }
    QByteArray encodeUnsubscribe() const
    {
        QJsonObject root;
        root["op"] = "unsub";
        root["type"] = this->type;
        root["condition"] = this->condition;
        return QJsonDocument(root).toJson();
    }

    friend QDebug &operator<<(QDebug &dbg,
                              const DummySubscription &subscription)
    {
        dbg << "DummySubscription{ condition:" << subscription.condition
            << "type:" << (int)subscription.type << '}';
        return dbg;
    }
};

}  // namespace

namespace std {
template <>
struct hash<DummySubscription> {
    size_t operator()(const DummySubscription &sub) const
    {
        return (size_t)qHash(sub.condition, qHash(sub.type));
    }
};
}  // namespace std

namespace {

class MyManager;
class MyClient : public BasicPubSubClient<DummySubscription, MyClient>
{
public:
    MyClient(MyManager &manager)
        : manager(manager)
    {
    }

    void onMessage(const QByteArray &msg) /* override */;

private:
    MyManager &manager;
};

class MyManager : public BasicPubSubManager<MyManager, MyClient>
{
public:
    MyManager(QString host)
        : BasicPubSubManager(std::move(host), "Test")
    {
    }

    std::atomic<int32_t> messagesReceived{0};

    std::optional<QString> popMessage()
    {
        std::lock_guard<std::mutex> guard(this->messageMtx_);
        if (this->messageQueue_.empty())
        {
            return std::nullopt;
        }
        QString front = this->messageQueue_.front();
        this->messageQueue_.pop_front();
        return front;
    }

    void sub(const DummySubscription &sub)
    {
        // We don't track subscriptions in this test
        this->subscribe(sub);
    }

    void unsub(const DummySubscription &sub)
    {
        this->unsubscribe(sub);
    }

    std::shared_ptr<MyClient> makeClient()
    {
        return std::make_shared<MyClient>(*this);
    }

private:
    std::mutex messageMtx_;
    std::deque<QString> messageQueue_;

    friend MyClient;
};

void MyClient::onMessage(const QByteArray &msg)
{
    std::lock_guard<std::mutex> guard(this->manager.messageMtx_);
    this->manager.messagesReceived.fetch_add(1, std::memory_order_acq_rel);
    this->manager.messageQueue_.emplace_back(QString::fromUtf8(msg));
}

}  // namespace

TEST(BasicPubSub, SubscriptionCycle)
{
    mock::BaseApplication app;
    const QString host("wss://127.0.0.1:9050/liveupdates/sub-unsub");
    MyManager manager(host);
    manager.sub({1, "foo"});
    QTest::qWait(500);
    ASSERT_EQ(manager.diag.connectionsClosed, 0);
    ASSERT_EQ(manager.diag.connectionsFailed, 0);
    ASSERT_EQ(manager.messagesReceived, 1);

    ASSERT_EQ(manager.popMessage(), QString("ack-sub-1-foo"));

    manager.unsub({1, "foo"});
    QTest::qWait(50);

    ASSERT_EQ(manager.diag.connectionsOpened, 1);
    ASSERT_EQ(manager.diag.connectionsClosed, 0);
    ASSERT_EQ(manager.diag.connectionsFailed, 0);
    ASSERT_EQ(manager.messagesReceived, 2);
    ASSERT_EQ(manager.popMessage(), QString("ack-unsub-1-foo"));

    manager.stop();
    // after exactly one event loop iteration, we should see updated counters
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    ASSERT_EQ(manager.diag.connectionsOpened, 1);
    ASSERT_EQ(manager.diag.connectionsClosed, 1);
    ASSERT_EQ(manager.diag.connectionsFailed, 0);
    ASSERT_EQ(manager.messagesReceived, 2);
}
