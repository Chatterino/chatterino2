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
    assert(file.open(QFile::ReadOnly));

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
    void onSessionWelcome(messages::Metadata metadata,
                          payload::session_welcome::Payload payload) override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&payload);
    }

    void onNotification(messages::Metadata metadata,
                        const boost::json::value &jv) override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&jv);
    }

    void onChannelBan(messages::Metadata metadata,
                      payload::channel_ban::v1::Payload payload) override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&payload);
    }

    void onStreamOnline(messages::Metadata metadata,
                        payload::stream_online::v1::Payload payload) override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&payload);
    }

    void onStreamOffline(messages::Metadata metadata,
                         payload::stream_offline::v1::Payload payload) override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&payload);
    }

    void onChannelChatNotification(
        messages::Metadata metadata,
        payload::channel_chat_notification::v1::Payload payload) override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&payload);
    }

    void onChannelUpdate(messages::Metadata metadata,
                         payload::channel_update::v1::Payload payload) override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&payload);
    }

    void onChannelChatMessage(
        messages::Metadata metadata,
        payload::channel_chat_message::v1::Payload payload) override
    {
        benchmark::DoNotOptimize(&metadata);
        benchmark::DoNotOptimize(&payload);
    }

    void onChannelModerate(
        messages::Metadata metadata,
        payload::channel_moderate::v2::Payload payload) override
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

    for (auto _ : state)
    {
        for (const auto &msg : messages)
        {
            boost::system::error_code ec = handleMessage(listener, msg);
            assert(!ec);
        }
    }
}

}  // namespace

BENCHMARK(BM_ParseAndHandleMessages);
