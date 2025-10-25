#include "common/Literals.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "messages/Emote.hpp"
#include "mocks/BaseApplication.hpp"
#include "mocks/DisabledStreamerMode.hpp"
#include "mocks/EmoteController.hpp"
#include "mocks/EmoteProvider.hpp"
#include "mocks/LinkResolver.hpp"
#include "mocks/Logging.hpp"
#include "mocks/TwitchIrcServer.hpp"
#include "mocks/UserData.hpp"
#include "providers/chatterino/ChatterinoBadges.hpp"
#include "providers/ffz/FfzBadges.hpp"
#include "providers/recentmessages/Impl.hpp"
#include "providers/seventv/SeventvBadges.hpp"
#include "providers/seventv/SeventvEmoteProvider.hpp"
#include "providers/twitch/TwitchBadges.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Resources.hpp"

#include <benchmark/benchmark.h>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QString>

#include <optional>

using namespace chatterino;
using namespace literals;

namespace {

class MockApplication : public mock::BaseApplication
{
public:
    MockApplication()
        : highlights(this->settings, &this->accounts)
    {
    }

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
    mock::EmoteController emoteController;
    ChatterinoBadges chatterinoBadges;
    FfzBadges ffzBadges;
    SeventvBadges seventvBadges;
    HighlightController highlights;
    TwitchBadges twitchBadges;
    DisabledStreamerMode streamerMode;
};

std::optional<QJsonDocument> tryReadJsonFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly))
    {
        return std::nullopt;
    }

    QJsonParseError e;
    auto doc = QJsonDocument::fromJson(file.readAll(), &e);
    if (e.error != QJsonParseError::NoError)
    {
        return std::nullopt;
    }

    return doc;
}

QJsonDocument readJsonFile(const QString &path)
{
    auto opt = tryReadJsonFile(path);
    if (!opt)
    {
        _exit(1);
    }
    return *opt;
}

class RecentMessages
{
public:
    explicit RecentMessages(const QString &name_)
        : name(name_)
        , chan(this->name)
    {
        const auto seventvEmotes =
            tryReadJsonFile(u":/bench/seventvemotes-%1.json"_s.arg(this->name));

        if (seventvEmotes)
        {
            auto map = std::make_shared<EmoteMap>();
            for (const auto el : seventvEmotes->object()["emote_set"_L1]
                                     .toObject()["emotes"_L1]
                                     .toArray())
            {
                auto emote = seventv::detail::parseEmote(el.toObject(), false);
                if (!emote)
                {
                    continue;
                }
                auto name = emote->name;
                auto ptr = std::make_shared<const Emote>(*std::move(emote));
                (*map)[name] = std::move(ptr);
            }

            auto provider =
                std::make_shared<mock::EmoteProvider>("7TV", "seventv", 3);
            provider->setChannelFallback(std::move(map));
            this->app.emoteController.addProvider(provider);
        }

        this->messages =
            readJsonFile(u":/bench/recentmessages-%1.json"_s.arg(this->name));
    }

    ~RecentMessages()
    {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    virtual void run(benchmark::State &state) = 0;

protected:
    QString name;
    MockApplication app;
    TwitchChannel chan;
    QJsonDocument messages;
};

class ParseRecentMessages : public RecentMessages
{
public:
    explicit ParseRecentMessages(const QString &name_)
        : RecentMessages(name_)
    {
    }

    void run(benchmark::State &state)
    {
        for (auto _ : state)
        {
            auto parsed = recentmessages::detail::parseRecentMessages(
                this->messages.object());
            benchmark::DoNotOptimize(parsed);
        }
    }
};

class BuildRecentMessages : public RecentMessages
{
public:
    explicit BuildRecentMessages(const QString &name_)
        : RecentMessages(name_)
    {
    }

    void run(benchmark::State &state)
    {
        auto parsed = recentmessages::detail::parseRecentMessages(
            this->messages.object());
        for (auto _ : state)
        {
            auto built = recentmessages::detail::buildRecentMessages(
                parsed, &this->chan);
            benchmark::DoNotOptimize(built);
        }
    }
};

void BM_ParseRecentMessages(benchmark::State &state, const QString &name)
{
    ParseRecentMessages bench(name);
    bench.run(state);
}

void BM_BuildRecentMessages(benchmark::State &state, const QString &name)
{
    BuildRecentMessages bench(name);
    bench.run(state);
}

}  // namespace

BENCHMARK_CAPTURE(BM_ParseRecentMessages, nymn, u"nymn"_s);
BENCHMARK_CAPTURE(BM_BuildRecentMessages, nymn, u"nymn"_s);
