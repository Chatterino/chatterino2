#include "common/websockets/WebSocketPool.hpp"

#include "mocks/BaseApplication.hpp"
#include "Test.hpp"
#include "util/OnceFlag.hpp"

using namespace chatterino;
using namespace std::chrono_literals;

namespace {

struct Listener : public WebSocketListener {
    Listener(std::vector<std::pair<bool, QByteArray>> &messages,
             OnceFlag &messageFlag, OnceFlag &closeFlag)
        : messages(messages)
        , messageFlag(messageFlag)
        , closeFlag(closeFlag)
    {
    }

    void onClose(std::unique_ptr<WebSocketListener> /*self*/) override
    {
        this->closeFlag.set();
    }

    void onTextMessage(QByteArray data) override
    {
        this->messageFlag.set();
        messages.emplace_back(true, std::move(data));
    }

    void onBinaryMessage(QByteArray data) override
    {
        // no flag to know when the initial queue is empty
        messages.emplace_back(false, std::move(data));
    }

    std::vector<std::pair<bool, QByteArray>> &messages;
    OnceFlag &messageFlag;
    OnceFlag &closeFlag;
};

}  // namespace

TEST(WebSocketPool, tcpEcho)
{
    mock::BaseApplication app;
    WebSocketPool pool;

    std::vector<std::pair<bool, QByteArray>> messages;
    OnceFlag messageFlag;
    OnceFlag closeFlag;

    auto handle = pool.createSocket(
        {
            .url = QUrl("ws://127.0.0.1:9052/echo"),
            .headers =
                {
                    {"My-Header", "my-header-VALUE"},
                    {"Another-Header", "other-header"},
                    {"Cookie", "xd"},  // "known" header
                    {"User-Agent", "MyUserAgent"},
                },
        },
        std::make_unique<Listener>(messages, messageFlag, closeFlag));
    handle.sendBinary("message1");
    handle.sendBinary("message2");
    handle.sendBinary("message3");
    handle.sendText("message4");

    ASSERT_TRUE(messageFlag.waitFor(1s));
    QByteArray bigMsg(1 << 15, 'A');
    handle.sendBinary(bigMsg);
    handle.sendText("foo");
    handle.sendText("/HEADER my-header");
    handle.sendText("/HEADER another-header");
    handle.sendText("/HEADER cookie");
    handle.sendText("/HEADER user-agent");
    handle.sendText("/CLOSE");

    ASSERT_TRUE(closeFlag.waitFor(1s));

    ASSERT_EQ(messages.size(), 10);
    ASSERT_EQ(messages[0].first, false);
    ASSERT_EQ(messages[0].second, "message1");
    ASSERT_EQ(messages[1].first, false);
    ASSERT_EQ(messages[1].second, "message2");
    ASSERT_EQ(messages[2].first, false);
    ASSERT_EQ(messages[2].second, "message3");
    ASSERT_EQ(messages[3].first, true);
    ASSERT_EQ(messages[3].second, "message4");
    ASSERT_EQ(messages[4].first, false);
    ASSERT_EQ(messages[4].second, bigMsg);
    ASSERT_EQ(messages[5].first, true);
    ASSERT_EQ(messages[5].second, "foo");
    ASSERT_EQ(messages[6].first, true);
    ASSERT_EQ(messages[6].second, "my-header-VALUE");
    ASSERT_EQ(messages[7].first, true);
    ASSERT_EQ(messages[7].second, "other-header");
    ASSERT_EQ(messages[8].first, true);
    ASSERT_EQ(messages[8].second, "xd");
    ASSERT_EQ(messages[9].first, true);
    ASSERT_EQ(messages[9].second, "MyUserAgent");
}

TEST(WebSocketPool, tlsEcho)
{
    mock::BaseApplication app;
    WebSocketPool pool;

    std::vector<std::pair<bool, QByteArray>> messages;
    OnceFlag messageFlag;
    OnceFlag closeFlag;

    auto handle = pool.createSocket(
        {
            .url = QUrl("wss://127.0.0.1:9050/echo"),
            .headers{
                {"My-Header", "my-header-VALUE"},
                {"Another-Header", "other-header"},
                {"Cookie", "xd"},  // "known" header
            },
        },
        std::make_unique<Listener>(messages, messageFlag, closeFlag));
    handle.sendBinary("message1");
    handle.sendBinary("message2");
    handle.sendBinary("message3");
    handle.sendText("message4");

    ASSERT_TRUE(messageFlag.waitFor(1s));
    QByteArray bigMsg(1 << 15, 'A');
    handle.sendBinary(bigMsg);
    handle.sendText("foo");
    handle.sendText("/HEADER my-header");
    handle.sendText("/HEADER another-header");
    handle.sendText("/HEADER cookie");
    handle.sendText("/HEADER user-agent");
    handle.sendText("/CLOSE");

    ASSERT_TRUE(closeFlag.waitFor(1s));

    ASSERT_EQ(messages.size(), 10);
    ASSERT_EQ(messages[0].first, false);
    ASSERT_EQ(messages[0].second, "message1");
    ASSERT_EQ(messages[1].first, false);
    ASSERT_EQ(messages[1].second, "message2");
    ASSERT_EQ(messages[2].first, false);
    ASSERT_EQ(messages[2].second, "message3");
    ASSERT_EQ(messages[3].first, true);
    ASSERT_EQ(messages[3].second, "message4");
    ASSERT_EQ(messages[4].first, false);
    ASSERT_EQ(messages[4].second, bigMsg);
    ASSERT_EQ(messages[5].first, true);
    ASSERT_EQ(messages[5].second, "foo");
    ASSERT_EQ(messages[6].first, true);
    ASSERT_EQ(messages[6].second, "my-header-VALUE");
    ASSERT_EQ(messages[7].first, true);
    ASSERT_EQ(messages[7].second, "other-header");
    ASSERT_EQ(messages[8].first, true);
    ASSERT_EQ(messages[8].second, "xd");
    ASSERT_EQ(messages[9].first, true);
    ASSERT_TRUE(messages[9].second.startsWith("Chatterino"));
}
