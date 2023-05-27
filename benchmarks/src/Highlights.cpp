#include "Application.hpp"
#include "common/Channel.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "controllers/highlights/HighlightPhrase.hpp"
#include "messages/Message.hpp"
#include "messages/SharedMessageBuilder.hpp"
#include "mocks/EmptyApplication.hpp"
#include "singletons/Settings.hpp"
#include "util/Helpers.hpp"

#include <benchmark/benchmark.h>
#include <QDebug>
#include <QString>
#include <QTemporaryDir>

using namespace chatterino;

class BenchmarkMessageBuilder : public SharedMessageBuilder
{
public:
    explicit BenchmarkMessageBuilder(
        Channel *_channel, const Communi::IrcPrivateMessage *_ircMessage,
        const MessageParseArgs &_args)
        : SharedMessageBuilder(_channel, _ircMessage, _args)
    {
    }
    virtual MessagePtr build()
    {
        // PARSE
        this->parse();
        this->usernameColor_ = getRandomColor(this->ircMessage->nick());

        // words
        // this->addWords(this->originalMessage_.split(' '));

        this->message().messageText = this->originalMessage_;
        this->message().searchText = this->message().localizedName + " " +
                                     this->userName + ": " +
                                     this->originalMessage_;
        return nullptr;
    }

    void bench()
    {
        this->parseHighlights();
    }
};

class MockApplication : mock::EmptyApplication
{
public:
    AccountController *getAccounts() override
    {
        return &this->accounts;
    }
    HighlightController *getHighlights() override
    {
        return &this->highlights;
    }

    AccountController accounts;
    HighlightController highlights;
    // TODO: Figure this out
};

static void BM_HighlightTest(benchmark::State &state)
{
    MockApplication mockApplication;
    QTemporaryDir settingsDir;
    Settings settings(settingsDir.path());

    std::string message =
        R"(@badge-info=subscriber/34;badges=moderator/1,subscriber/24;color=#FF0000;display-name=테스트계정420;emotes=41:6-13,15-22;flags=;id=a3196c7e-be4c-4b49-9c5a-8b8302b50c2a;mod=1;room-id=11148817;subscriber=1;tmi-sent-ts=1590922213730;turbo=0;user-id=117166826;user-type=mod :testaccount_420!testaccount_420@testaccount_420.tmi.twitch.tv PRIVMSG #pajlada :-tags Kreygasm,Kreygasm (no space))";
    auto ircMessage = Communi::IrcMessage::fromData(message.c_str(), nullptr);
    auto privMsg = dynamic_cast<Communi::IrcPrivateMessage *>(ircMessage);
    assert(privMsg != nullptr);
    MessageParseArgs args;
    auto emptyChannel = Channel::getEmpty();

    for (auto _ : state)
    {
        state.PauseTiming();
        BenchmarkMessageBuilder b(emptyChannel.get(), privMsg, args);

        b.build();
        state.ResumeTiming();

        b.bench();
    }
}

BENCHMARK(BM_HighlightTest);
