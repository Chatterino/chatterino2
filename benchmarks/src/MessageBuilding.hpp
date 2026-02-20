#pragma once

#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "mocks/BaseApplication.hpp"
#include "mocks/EmoteController.hpp"
#include "mocks/LinkResolver.hpp"
#include "mocks/Logging.hpp"
#include "mocks/TwitchIrcServer.hpp"
#include "mocks/UserData.hpp"
#include "providers/bttv/BttvBadges.hpp"
#include "providers/chatterino/ChatterinoBadges.hpp"
#include "providers/seventv/SeventvBadges.hpp"
#include "providers/twitch/TwitchBadges.hpp"

#include <benchmark/benchmark.h>

namespace chatterino::bench {

class MockMessageApplication : public mock::BaseApplication
{
public:
    MockMessageApplication();

    EmoteController *getEmotes() override
    {
        return &this->emotes;
    }

    IUserDataController *getUserData() override
    {
        return &this->userData;
    }

    AccountController *getAccounts() override
    {
        return &this->accounts;
    }

    ITwitchIrcServer *getTwitch() override
    {
        return &this->twitch;
    }

    ChatterinoBadges *getChatterinoBadges() override
    {
        return &this->chatterinoBadges;
    }

    FfzBadges *getFfzBadges() override
    {
        return &this->ffzBadges;
    }

    BttvBadges *getBttvBadges() override
    {
        return &this->bttvBadges;
    }

    SeventvBadges *getSeventvBadges() override
    {
        return &this->seventvBadges;
    }

    HighlightController *getHighlights() override
    {
        return &this->highlights;
    }

    TwitchBadges *getTwitchBadges() override
    {
        return &this->twitchBadges;
    }

    BttvEmotes *getBttvEmotes() override
    {
        return &this->bttvEmotes;
    }

    FfzEmotes *getFfzEmotes() override
    {
        return &this->ffzEmotes;
    }

    SeventvEmotes *getSeventvEmotes() override
    {
        return &this->seventvEmotes;
    }

    IStreamerMode *getStreamerMode() override
    {
        return &this->streamerMode;
    }

    ILinkResolver *getLinkResolver() override
    {
        return &this->linkResolver;
    }

    ILogging *getChatLogger() override
    {
        return &this->logging;
    }

    mock::EmptyLogging logging;
    AccountController accounts;
    mock::EmoteController emotes;
    mock::UserDataController userData;
    mock::MockTwitchIrcServer twitch;
    mock::EmptyLinkResolver linkResolver;
    ChatterinoBadges chatterinoBadges;
    FfzBadges ffzBadges;
    BttvBadges bttvBadges;
    SeventvBadges seventvBadges;
    HighlightController highlights;
    TwitchBadges twitchBadges;
    BttvEmotes bttvEmotes;
    FfzEmotes ffzEmotes;
    SeventvEmotes seventvEmotes;
    DisabledStreamerMode streamerMode;
};

class MessageBenchmark
{
public:
    explicit MessageBenchmark(QString name);

    virtual ~MessageBenchmark()
    {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    MessageBenchmark(const MessageBenchmark &) = delete;
    MessageBenchmark(MessageBenchmark &&) = delete;
    MessageBenchmark &operator=(const MessageBenchmark &) = delete;
    MessageBenchmark &operator=(MessageBenchmark &&) = delete;

    virtual void run(benchmark::State &state) = 0;

protected:
    QString name;
    MockMessageApplication app;
    std::shared_ptr<TwitchChannel> chan;
    QJsonDocument messages;
};

}  // namespace chatterino::bench
