#include "twitch-eventsub-ws/listener.hpp"
#include "twitch-eventsub-ws/session.hpp"

#include <boost/beast/core/flat_buffer.hpp>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace {

using namespace chatterino::eventsub::lib;

std::filesystem::path baseDir()
{
    std::filesystem::path snapshotDir(__FILE__);
    return snapshotDir.parent_path().parent_path() / "resources" / "messages";
}

std::filesystem::path filePath(std::string_view name)
{
    return baseDir() / name;
}

std::vector<std::string> discover()
{
    std::vector<std::string> files;
    for (const auto &entry : std::filesystem::directory_iterator{baseDir()})
    {
        if (entry.is_regular_file())
        {
            auto name = entry.path().filename().string();
            if (name.ends_with(".json"))
            {
                files.emplace_back(name.substr(0, name.length() - 5));
            }
        }
    }
    return files;
}

boost::beast::flat_buffer readToFlatBuffer(const std::filesystem::path &path)
{
    std::ifstream istream(path);
    std::ostringstream buffer;
    buffer << istream.rdbuf();

    boost::beast::flat_buffer buf;
    auto inner = buf.prepare(buffer.view().size());
    std::memcpy(inner.data(), buffer.view().data(), inner.size());
    buf.commit(inner.size());
    return buf;
}

class NoOpListener : public chatterino::eventsub::lib::Listener
{
    void onSessionWelcome(
        const messages::Metadata &metadata,
        const payload::session_welcome::Payload &payload) override
    {
    }

    void onNotification(const messages::Metadata &metadata,
                        const boost::json::value &jv) override
    {
    }

    void onChannelBan(const messages::Metadata &metadata,
                      const payload::channel_ban::v1::Payload &payload) override
    {
    }

    void onStreamOnline(
        const messages::Metadata &metadata,
        const payload::stream_online::v1::Payload &payload) override
    {
    }

    void onStreamOffline(
        const messages::Metadata &metadata,
        const payload::stream_offline::v1::Payload &payload) override
    {
    }

    void onChannelChatNotification(
        const messages::Metadata &metadata,
        const payload::channel_chat_notification::v1::Payload &payload) override
    {
    }

    void onChannelUpdate(
        const messages::Metadata &metadata,
        const payload::channel_update::v1::Payload &payload) override
    {
    }

    void onChannelChatMessage(
        const messages::Metadata &metadata,
        const payload::channel_chat_message::v1::Payload &payload) override
    {
    }

    void onChannelModerate(
        const messages::Metadata &metadata,
        const payload::channel_moderate::v2::Payload &payload) override
    {
    }

    void onAutomodMessageHold(
        const messages::Metadata &metadata,
        const payload::automod_message_hold::v2::Payload &payload) override
    {
    }
    void onAutomodMessageUpdate(
        const messages::Metadata &metadata,
        const payload::automod_message_update::v2::Payload &payload) override
    {
    }
    void onChannelSuspiciousUserMessage(
        const messages::Metadata &metadata,
        const payload::channel_suspicious_user_message::v1::Payload &payload)
        override
    {
    }
    void onChannelSuspiciousUserUpdate(
        const messages::Metadata &metadata,
        const payload::channel_suspicious_user_update::v1::Payload &payload)
        override
    {
    }
    void onChannelChatUserMessageHold(
        const messages::Metadata &metadata,
        const payload::channel_chat_user_message_hold::v1::Payload &payload)
        override
    {
    }
    void onChannelChatUserMessageUpdate(
        const messages::Metadata &metadata,
        const payload::channel_chat_user_message_update::v1::Payload &payload)
        override
    {
    }
};

}  // namespace

class TestHandleMessageP : public ::testing::TestWithParam<std::string>
{
};

TEST_P(TestHandleMessageP, Run)
{
    auto buf = readToFlatBuffer(filePath(GetParam() + ".json"));

    std::unique_ptr<Listener> listener = std::make_unique<NoOpListener>();
    boost::asio::io_context ioc;
    boost::asio::ssl::context ssl(
        boost::asio::ssl::context::method::tls_client);
    auto sess = std::make_shared<Session>(ioc, ssl, std::move(listener));
    auto ec = sess->handleMessage(buf);
    ASSERT_FALSE(ec.failed())
        << ec.what() << ec.message() << ec.location().to_string();
}

INSTANTIATE_TEST_SUITE_P(HandleMessage, TestHandleMessageP,
                         testing::ValuesIn(discover()));
