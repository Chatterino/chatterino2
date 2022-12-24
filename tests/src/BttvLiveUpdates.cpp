#include "providers/bttv/BttvLiveUpdates.hpp"

#include <boost/optional.hpp>
#include <gtest/gtest.h>
#include <QString>

using namespace chatterino;
using namespace std::chrono_literals;

const QString TARGET_USER_ID = "1234567";
const QString TARGET_USER_NAME = "Alien";

TEST(BttvLiveUpdates, AllEvents)
{
    const QString host("wss://127.0.0.1:9050/liveupdates/bttv/all-events");
    auto *liveUpdates = new BttvLiveUpdates(host);
    liveUpdates->start();

    boost::optional<BttvLiveUpdateEmoteUpdateAddMessage> addMessage;
    boost::optional<BttvLiveUpdateEmoteUpdateAddMessage> updateMessage;
    boost::optional<BttvLiveUpdateEmoteRemoveMessage> removeMessage;

    liveUpdates->signals_.emoteAdded.connect([&](const auto &m) {
        addMessage = m;
    });
    liveUpdates->signals_.emoteUpdated.connect([&](const auto &m) {
        updateMessage = m;
    });
    liveUpdates->signals_.emoteRemoved.connect([&](const auto &m) {
        removeMessage = m;
    });

    std::this_thread::sleep_for(50ms);
    liveUpdates->joinChannel(TARGET_USER_ID, TARGET_USER_NAME);
    std::this_thread::sleep_for(500ms);

    ASSERT_EQ(liveUpdates->diag.connectionsOpened, 1);
    ASSERT_EQ(liveUpdates->diag.connectionsClosed, 0);
    ASSERT_EQ(liveUpdates->diag.connectionsFailed, 0);

    auto add = *addMessage;
    ASSERT_EQ(add.channelID, TARGET_USER_ID);
    ASSERT_EQ(add.emoteName, QString("PepePls"));
    ASSERT_EQ(add.emoteID, QString("55898e122612142e6aaa935b"));

    auto update = *updateMessage;
    ASSERT_EQ(update.channelID, TARGET_USER_ID);
    ASSERT_EQ(update.emoteName, QString("PepePls"));
    ASSERT_EQ(update.emoteID, QString("55898e122612142e6aaa935b"));

    auto rem = *removeMessage;
    ASSERT_EQ(rem.channelID, TARGET_USER_ID);
    ASSERT_EQ(rem.emoteID, QString("55898e122612142e6aaa935b"));

    liveUpdates->stop();
    ASSERT_EQ(liveUpdates->diag.connectionsOpened, 1);
    ASSERT_EQ(liveUpdates->diag.connectionsClosed, 1);
    ASSERT_EQ(liveUpdates->diag.connectionsFailed, 0);
}
