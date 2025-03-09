#include "twitch-eventsub-ws/listener.hpp"
#include "twitch-eventsub-ws/session.hpp"

#include <benchmark/benchmark.h>
#include <boost/beast/core/flat_buffer.hpp>
#include <QFile>

#include <memory>

namespace {

using namespace chatterino::eventsub::lib;

std::vector<boost::beast::flat_buffer> readMessages()
{
    QFile file(":/bench/messages.ndjson");
    bool ok = file.open(QFile::ReadOnly);
    assert(ok);

    std::vector<boost::beast::flat_buffer> messages;
    while (!file.atEnd())
    {
        QByteArray line = file.readLine();
        if (line.isEmpty())
        {
            continue;
        }

        boost::beast::flat_buffer buf;
        auto inner = buf.prepare(line.size());
        std::memcpy(inner.data(), line.data(), inner.size());
        buf.commit(inner.size());

        messages.emplace_back(std::move(buf));
    }
    return messages;
}

class NoopListener : public Listener
{
public:
    NoopListener() = default;

    // NOLINTBEGIN(cppcoreguidelines-pro-type-const-cast)
    void onSessionWelcome(
        const messages::Metadata &metadata,
        const payload::session_welcome::Payload &payload) override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&payload);
    }

    void onNotification(const messages::Metadata &metadata,
                        const boost::json::value &jv) override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&jv);
    }

    void onChannelBan(const messages::Metadata &metadata,
                      const payload::channel_ban::v1::Payload &payload) override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&payload);
    }

    void onStreamOnline(
        const messages::Metadata &metadata,
        const payload::stream_online::v1::Payload &payload) override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&payload);
    }

    void onStreamOffline(
        const messages::Metadata &metadata,
        const payload::stream_offline::v1::Payload &payload) override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&payload);
    }

    void onChannelChatNotification(
        const messages::Metadata &metadata,
        const payload::channel_chat_notification::v1::Payload &payload) override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&payload);
    }

    void onChannelUpdate(
        const messages::Metadata &metadata,
        const payload::channel_update::v1::Payload &payload) override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&payload);
    }

    void onChannelChatMessage(
        const messages::Metadata &metadata,
        const payload::channel_chat_message::v1::Payload &payload) override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&payload);
    }

    void onChannelModerate(
        const messages::Metadata &metadata,
        const payload::channel_moderate::v2::Payload &payload) override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&payload);
    }

    void onAutomodMessageHold(
        const messages::Metadata &metadata,
        const payload::automod_message_hold::v2::Payload &payload) override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&payload);
    }

    void onAutomodMessageUpdate(
        const messages::Metadata &metadata,
        const payload::automod_message_update::v2::Payload &payload) override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&payload);
    }

    void onChannelSuspiciousUserMessage(
        const messages::Metadata &metadata,
        const payload::channel_suspicious_user_message::v1::Payload &payload)
        override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&payload);
    }

    void onChannelSuspiciousUserUpdate(
        const messages::Metadata &metadata,
        const payload::channel_suspicious_user_update::v1::Payload &payload)
        override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&payload);
    }

    void onChannelChatUserMessageHold(
        const messages::Metadata &metadata,
        const payload::channel_chat_user_message_hold::v1::Payload &payload)
        override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&payload);
    }

    void onChannelChatUserMessageUpdate(
        const messages::Metadata &metadata,
        const payload::channel_chat_user_message_update::v1::Payload &payload)
        override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&payload);
    }
    // NOLINTEND(cppcoreguidelines-pro-type-const-cast)
};

void BM_ParseAndHandleMessages(benchmark::State &state)
{
    auto messages = readMessages();

    std::unique_ptr<Listener> listener = std::make_unique<NoopListener>();
    boost::asio::io_context ioc;
    boost::asio::ssl::context ssl(
        boost::asio::ssl::context::method::tls_client);
    auto sess = std::make_shared<Session>(ioc, ssl, std::move(listener));

    for (auto _ : state)
    {
        for (const auto &msg : messages)
        {
            boost::system::error_code ec = sess->handleMessage(msg);
            assert(!ec);
        }
    }
}

}  // namespace

BENCHMARK(BM_ParseAndHandleMessages);
