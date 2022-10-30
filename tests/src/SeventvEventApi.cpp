#include "providers/seventv/SeventvEventApi.hpp"

#include <gtest/gtest.h>
#include <QString>
#include <boost/optional.hpp>

using namespace chatterino;
using namespace std::chrono_literals;

const QString EMOTE_SET_A = "60b39e943e203cc169dfc106";
const QString EMOTE_SET_B = "60bca831e7ecd2f892c9b9ab";
const QString TARGET_USER_ID = "60b39e943e203cc169dfc106";

TEST(SeventvEventApi, AllEvents)
{
    const QString host("wss://127.0.0.1:9050/liveupdates/seventv/all-events");
    auto *eventApi = new SeventvEventApi(host, std::chrono::milliseconds(1000));
    eventApi->start();

    boost::optional<SeventvEventApiEmoteAddDispatch> addDispatch;
    boost::optional<SeventvEventApiEmoteUpdateDispatch> updateDispatch;
    boost::optional<SeventvEventApiEmoteRemoveDispatch> removeDispatch;
    boost::optional<SeventvEventApiUserConnectionUpdateDispatch> userDispatch;

    eventApi->signals_.emoteAdded.connect([&](const auto &d) {
        addDispatch = d;
    });
    eventApi->signals_.emoteUpdated.connect([&](const auto &d) {
        updateDispatch = d;
    });
    eventApi->signals_.emoteRemoved.connect([&](const auto &d) {
        removeDispatch = d;
    });
    eventApi->signals_.userUpdated.connect([&](const auto &d) {
        userDispatch = d;
    });

    std::this_thread::sleep_for(50ms);
    eventApi->subscribeUser("", EMOTE_SET_A);
    std::this_thread::sleep_for(500ms);

    ASSERT_EQ(eventApi->diag.connectionsOpened, 1);
    ASSERT_EQ(eventApi->diag.connectionsClosed, 0);
    ASSERT_EQ(eventApi->diag.connectionsFailed, 0);

    auto add = *addDispatch;
    ASSERT_EQ(add.emoteSetId, EMOTE_SET_A);
    ASSERT_EQ(add.actorName, QString("nerixyz"));
    ASSERT_EQ(add.emoteId, QString("621d13967cc2d4e1953838ed"));

    auto upd = *updateDispatch;
    ASSERT_EQ(upd.emoteSetId, EMOTE_SET_A);
    ASSERT_EQ(upd.actorName, QString("nerixyz"));
    ASSERT_EQ(upd.emoteId, QString("621d13967cc2d4e1953838ed"));
    ASSERT_EQ(upd.oldEmoteName, QString("Chatterinoge"));
    ASSERT_EQ(upd.emoteName, QString("Chatterino"));

    auto rem = *removeDispatch;
    ASSERT_EQ(rem.emoteSetId, EMOTE_SET_A);
    ASSERT_EQ(rem.actorName, QString("nerixyz"));
    ASSERT_EQ(rem.emoteName, QString("Chatterino"));
    ASSERT_EQ(rem.emoteId, QString("621d13967cc2d4e1953838ed"));

    ASSERT_EQ(userDispatch.has_value(), false);
    addDispatch = boost::none;
    updateDispatch = boost::none;
    removeDispatch = boost::none;

    eventApi->subscribeUser(TARGET_USER_ID, "");
    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(addDispatch.has_value(), false);
    ASSERT_EQ(updateDispatch.has_value(), false);
    ASSERT_EQ(removeDispatch.has_value(), false);

    auto user = *userDispatch;
    ASSERT_EQ(user.userId, TARGET_USER_ID);
    ASSERT_EQ(user.actorName, QString("nerixyz"));
    ASSERT_EQ(user.oldEmoteSetId, EMOTE_SET_A);
    ASSERT_EQ(user.emoteSetId, EMOTE_SET_B);

    eventApi->stop();
    ASSERT_EQ(eventApi->diag.connectionsOpened, 1);
    ASSERT_EQ(eventApi->diag.connectionsClosed, 1);
    ASSERT_EQ(eventApi->diag.connectionsFailed, 0);
}

TEST(SeventvEventApi, NoHeartbeat)
{
    const QString host("wss://127.0.0.1:9050/liveupdates/seventv/no-heartbeat");
    auto *eventApi = new SeventvEventApi(host, std::chrono::milliseconds(1000));
    eventApi->start();

    std::this_thread::sleep_for(50ms);
    eventApi->subscribeUser("", EMOTE_SET_A);
    std::this_thread::sleep_for(1250ms);
    ASSERT_EQ(eventApi->diag.connectionsOpened, 2);
    ASSERT_EQ(eventApi->diag.connectionsClosed, 1);
    ASSERT_EQ(eventApi->diag.connectionsFailed, 0);

    eventApi->stop();
}
