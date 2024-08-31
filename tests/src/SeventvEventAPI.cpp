#include "providers/seventv/SeventvEventAPI.hpp"

#include "providers/seventv/eventapi/Client.hpp"
#include "providers/seventv/eventapi/Dispatch.hpp"
#include "providers/seventv/eventapi/Message.hpp"
#include "Test.hpp"

#include <QString>

#include <optional>

using namespace chatterino;
using namespace chatterino::seventv::eventapi;
using namespace std::chrono_literals;

const QString EMOTE_SET_A = "60b39e943e203cc169dfc106";
const QString EMOTE_SET_B = "60bca831e7ecd2f892c9b9ab";
const QString TARGET_USER_ID = "60b39e943e203cc169dfc106";

TEST(SeventvEventAPI, AllEvents)
{
    const QString host("wss://127.0.0.1:9050/liveupdates/seventv/all-events");
    SeventvEventAPI eventAPI(host, std::chrono::milliseconds(1000));
    eventAPI.start();

    std::optional<EmoteAddDispatch> addDispatch;
    std::optional<EmoteUpdateDispatch> updateDispatch;
    std::optional<EmoteRemoveDispatch> removeDispatch;
    std::optional<UserConnectionUpdateDispatch> userDispatch;

    std::ignore = eventAPI.signals_.emoteAdded.connect([&](const auto &d) {
        addDispatch = d;
    });
    std::ignore = eventAPI.signals_.emoteUpdated.connect([&](const auto &d) {
        updateDispatch = d;
    });
    std::ignore = eventAPI.signals_.emoteRemoved.connect([&](const auto &d) {
        removeDispatch = d;
    });
    std::ignore = eventAPI.signals_.userUpdated.connect([&](const auto &d) {
        userDispatch = d;
    });

    std::this_thread::sleep_for(50ms);
    eventAPI.subscribeUser("", EMOTE_SET_A);
    std::this_thread::sleep_for(500ms);

    ASSERT_EQ(eventAPI.diag.connectionsOpened, 1);
    ASSERT_EQ(eventAPI.diag.connectionsClosed, 0);
    ASSERT_EQ(eventAPI.diag.connectionsFailed, 0);

    auto add = *addDispatch;
    ASSERT_EQ(add.emoteSetID, EMOTE_SET_A);
    ASSERT_EQ(add.actorName, QString("nerixyz"));
    ASSERT_EQ(add.emoteID, QString("621d13967cc2d4e1953838ed"));

    auto upd = *updateDispatch;
    ASSERT_EQ(upd.emoteSetID, EMOTE_SET_A);
    ASSERT_EQ(upd.actorName, QString("nerixyz"));
    ASSERT_EQ(upd.emoteID, QString("621d13967cc2d4e1953838ed"));
    ASSERT_EQ(upd.oldEmoteName, QString("Chatterinoge"));
    ASSERT_EQ(upd.emoteName, QString("Chatterino"));

    auto rem = *removeDispatch;
    ASSERT_EQ(rem.emoteSetID, EMOTE_SET_A);
    ASSERT_EQ(rem.actorName, QString("nerixyz"));
    ASSERT_EQ(rem.emoteName, QString("Chatterino"));
    ASSERT_EQ(rem.emoteID, QString("621d13967cc2d4e1953838ed"));

    ASSERT_EQ(userDispatch.has_value(), false);
    addDispatch = std::nullopt;
    updateDispatch = std::nullopt;
    removeDispatch = std::nullopt;

    eventAPI.subscribeUser(TARGET_USER_ID, "");
    std::this_thread::sleep_for(50ms);

    ASSERT_EQ(addDispatch.has_value(), false);
    ASSERT_EQ(updateDispatch.has_value(), false);
    ASSERT_EQ(removeDispatch.has_value(), false);

    auto user = *userDispatch;
    ASSERT_EQ(user.userID, TARGET_USER_ID);
    ASSERT_EQ(user.actorName, QString("nerixyz"));
    ASSERT_EQ(user.oldEmoteSetID, EMOTE_SET_A);
    ASSERT_EQ(user.emoteSetID, EMOTE_SET_B);
    ASSERT_EQ(user.connectionIndex, 0);

    eventAPI.stop();
    ASSERT_EQ(eventAPI.diag.connectionsOpened, 1);
    ASSERT_EQ(eventAPI.diag.connectionsClosed, 1);
    ASSERT_EQ(eventAPI.diag.connectionsFailed, 0);
}

TEST(SeventvEventAPI, NoHeartbeat)
{
    const QString host("wss://127.0.0.1:9050/liveupdates/seventv/no-heartbeat");
    SeventvEventAPI eventApi(host, std::chrono::milliseconds(1000));
    eventApi.start();

    std::this_thread::sleep_for(50ms);
    eventApi.subscribeUser("", EMOTE_SET_A);
    std::this_thread::sleep_for(1250ms);
    ASSERT_EQ(eventApi.diag.connectionsOpened, 2);
    ASSERT_EQ(eventApi.diag.connectionsClosed, 1);
    ASSERT_EQ(eventApi.diag.connectionsFailed, 0);

    eventApi.stop();
}
