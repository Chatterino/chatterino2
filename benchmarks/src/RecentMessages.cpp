#include "common/Literals.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "messages/Emote.hpp"
#include "mocks/EmptyApplication.hpp"
#include "mocks/TwitchIrcServer.hpp"
#include "mocks/UserData.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/chatterino/ChatterinoBadges.hpp"
#include "providers/ffz/FfzBadges.hpp"
#include "providers/ffz/FfzEmotes.hpp"
#include "providers/recentmessages/Impl.hpp"
#include "providers/seventv/SeventvBadges.hpp"
#include "providers/seventv/SeventvEmotes.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Emotes.hpp"
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

class MockApplication : mock::EmptyApplication
{
public:
    IEmotes *getEmotes() override
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

    AccountController accounts;
    Emotes emotes;
    mock::UserDataController userData;
    mock::MockTwitchIrcServer twitch;
    ChatterinoBadges chatterinoBadges;
    FfzBadges ffzBadges;
    SeventvBadges seventvBadges;
    HighlightController highlights;
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::unique_ptr<MockApplication> APP;

void doSetup(const benchmark::State & /*state*/)
{
    APP = std::make_unique<MockApplication>();
}

void doTeardown(const benchmark::State & /*state*/)
{
    APP = {};
}

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

void BM_RecentMessages(benchmark::State &state, const QString &name)
{
    initResources();
    TwitchChannel chan("nymn");

    const auto seventvEmotes =
        tryReadJsonFile(u":/bench/seventvemotes-%1.json"_s.arg(name));
    const auto bttvEmotes =
        tryReadJsonFile(u":/bench/bttvemotes-%1.json"_s.arg(name));
    const auto ffzEmotes =
        tryReadJsonFile(u":/bench/ffzemotes-%1.json"_s.arg(name));

    if (seventvEmotes)
    {
        chan.setSeventvEmotes(
            std::make_shared<const EmoteMap>(seventv::detail::parseEmotes(
                seventvEmotes->object()["emote_set"_L1]["emotes"_L1].toArray(),
                false)));
    }

    if (bttvEmotes)
    {
        chan.setBttvEmotes(std::make_shared<const EmoteMap>(
            bttv::detail::parseChannelEmotes(bttvEmotes->object(), name)));
    }

    if (ffzEmotes)
    {
        chan.setFfzEmotes(std::make_shared<const EmoteMap>(
            ffz::detail::parseChannelEmotes(ffzEmotes->object())));
    }

    auto messages = readJsonFile(u":/bench/recentmessages-%1.json"_s.arg(name));

    for (auto _ : state)
    {
        auto parsed =
            recentmessages::detail::parseRecentMessages(messages.object());
        auto built = recentmessages::detail::buildRecentMessages(parsed, &chan);
        benchmark::DoNotOptimize(built);
    }
}

}  // namespace

BENCHMARK_CAPTURE(BM_RecentMessages, nymn, u"nymn"_s)
    ->Setup(doSetup)
    ->Teardown(doTeardown);
