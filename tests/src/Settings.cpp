// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"
#include "controllers/sound/NullBackend.hpp"
#include "lib/Snapshot.hpp"
#include "messages/Emote.hpp"
#include "messages/Message.hpp"
#include "mocks/BaseApplication.hpp"
#include "mocks/ChatterinoBadges.hpp"
#include "mocks/DisabledStreamerMode.hpp"
#include "mocks/EmoteController.hpp"
#include "mocks/LinkResolver.hpp"
#include "mocks/Logging.hpp"
#include "mocks/TwitchIrcServer.hpp"
#include "mocks/UserData.hpp"
#include "providers/bttv/BttvBadges.hpp"
#include "providers/ffz/FfzBadges.hpp"
#include "providers/seventv/SeventvBadges.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/ChannelPointReward.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchBadge.hpp"
#include "providers/twitch/TwitchBadges.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "Test.hpp"
#include "util/IrcHelpers.hpp"
#include "util/VectorMessageSink.hpp"

#include <IrcConnection>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QStringBuilder>
#include <qtenvironmentvariables.h>

#include <unordered_map>
#include <vector>

using namespace chatterino;
using namespace Qt::StringLiterals;

namespace {

/// Controls whether snapshots will be updated (true) or verified (false)
///
/// In CI, all snapshots must be verified, thus the integrity tests checks for
/// this constant.
///
/// When adding a test, start with `{ "input": "..." }` and set this to `true`
/// to generate an initial snapshot. Make sure to verify the output!
const bool UPDATE_SNAPSHOTS =
    qEnvironmentVariableIsSet("CHATTERINO_UPDATE_TEST_SNAPSHOTS");

const QString TEST_CATEGORY = u"Settings"_s;

class MockApplication : public mock::EmptyApplication
{
public:
    MockApplication(const QString &settingsData)
        : mock::EmptyApplication(settingsData)
        , settings(this->args_, this->settingsDir.path(),
                   {
                       .isTest = true,
                       .runMigrations = true,
                   })
        , updates(this->paths_, this->settings)
    {
    }

    Updates &getUpdates() override
    {
        return this->updates;
    }

    Settings settings;
    Updates updates;
};

}  // namespace

class TestSettingsP : public ::testing::TestWithParam<QString>
{
public:
    void SetUp() override
    {
        auto param = TestSettingsP::GetParam();
        this->snapshot = testlib::Snapshot::read(TEST_CATEGORY, param);
    }

    void TearDown() override
    {
        this->snapshot.reset();
    }

    std::unique_ptr<testlib::Snapshot> snapshot;
};

TEST_P(TestSettingsP, Run)
{
    QJsonObject got;

    MockApplication app(
        QJsonDocument(this->snapshot->input().toObject()).toJson());

    ASSERT_EQ(app.settings.requestSave(),
              pajlada::Settings::SettingManager::SaveResult::Success);

    QFile settingsFile(app.settingsDir.filePath("settings.json"));
    ASSERT_TRUE(settingsFile.open(QFile::ReadOnly))
        << "failed to open" << app.settingsDir.filePath("settings.json");
    auto content = settingsFile.readAll();
    settingsFile.close();

    got = QJsonDocument::fromJson(content).object();

    ASSERT_TRUE(snapshot->run(got, UPDATE_SNAPSHOTS))
        << "Snapshot " << snapshot->name() << " failed. Expected JSON to be\n"
        << QJsonDocument(snapshot->output().toArray()).toJson() << "\nbut got\n"
        << QJsonDocument(got).toJson() << "\ninstead.";
}

INSTANTIATE_TEST_SUITE_P(
    SettingsMigration, TestSettingsP,
    testing::ValuesIn(testlib::Snapshot::discover(TEST_CATEGORY)));

TEST(TestSettingsP, Integrity)
{
    ASSERT_FALSE(UPDATE_SNAPSHOTS);  // make sure fixtures are actually tested
}

TEST(Settings, Bing)
{
#if 0
    {
        MockApplication app(R"({
            "misc": {"settingsVersion": 1},
            "pajlada": {
              "id": "forsen"
            }
        })");

        auto h = app.settings.pajlada.getValue();

        ASSERT_EQ(h.id, "forsen");
        ASSERT_TRUE(h.name.isNull());
    }

    {
        MockApplication app(R"({
            "pajlada": {
              "id": "forsen",
              "name": ""
            }
        })");

        auto h = app.settings.pajlada.getValue();

        ASSERT_EQ(h.id, "forsen");
        ASSERT_FALSE(h.name.isNull());
        ASSERT_EQ(h.name, "");
    }

    {
        MockApplication app(R"({
            "misc": {"settingsVersion": 1},
            "pajlada": {
              "id": "forsen",
              "name": "fors"
            }
        })");

        auto h = app.settings.pajlada.getValue();

        ASSERT_EQ(h.id, "forsen");
        ASSERT_FALSE(h.name.isNull());
        ASSERT_EQ(h.name, "fors");

        h.enabled = true;

        app.settings.pajlada = h;

        ASSERT_EQ(app.settings.requestSave(),
                  pajlada::Settings::SettingManager::SaveResult::Success);

        QFile settingsFile(app.settingsDir.filePath("settings.json"));
        ASSERT_TRUE(settingsFile.open(QFile::ReadOnly))
            << "failed to open" << app.settingsDir.filePath("settings.json");
        auto content = settingsFile.readAll();
        settingsFile.close();

        auto actual = QJsonDocument::fromJson(content);

        QJsonDocument expected;
        expected.setObject(QJsonObject{
            {"misc", QJsonObject{{"settingsVersion", 1}}},
            {
                "pajlada",
                QJsonObject{
                    {"id", "forsen"},
                    {"name", "fors"},
                    {"enabled", true},
                },
            },
        });

        ASSERT_EQ(expected, actual) << "expected:" << expected.toJson()
                                    << "\nactual:" << actual.toJson();
    }

    {
        MockApplication app(R"({
            "misc": {"settingsVersion": 1},
            "pajlada2": [
              {
                "id": "forsen",
                "name": "fors"
              }
            ]
        })");

        auto highlights = app.settings.pajlada2.getValue();

        ASSERT_EQ(highlights[0].id, "forsen");
        ASSERT_FALSE(highlights[0].name.isNull());
        ASSERT_EQ(highlights[0].name, "fors");

        highlights[0].enabled = true;

        app.settings.pajlada2 = highlights;

        ASSERT_EQ(app.settings.requestSave(),
                  pajlada::Settings::SettingManager::SaveResult::Success);

        QFile settingsFile(app.settingsDir.filePath("settings.json"));
        ASSERT_TRUE(settingsFile.open(QFile::ReadOnly))
            << "failed to open" << app.settingsDir.filePath("settings.json");
        auto content = settingsFile.readAll();
        settingsFile.close();

        auto actual = QJsonDocument::fromJson(content);

        QJsonDocument expected;
        expected.setObject(QJsonObject{
            {"misc", QJsonObject{{"settingsVersion", 1}}},
            {
                "pajlada2",
                QJsonArray{
                    QJsonObject{
                        {"id", "forsen"},
                        {"name", "fors"},
                        {"enabled", true},
                    },
                },
            },
        });

        ASSERT_EQ(expected, actual) << "expected:" << expected.toJson()
                                    << "\nactual:" << actual.toJson();
    }

    /*
    ASSERT_TRUE(snapshot->run(got, UPDATE_SNAPSHOTS))
        << "Snapshot " << snapshot->name() << " failed. Expected JSON to be\n"
        << QJsonDocument(snapshot->output().toArray()).toJson() << "\nbut got\n"
        << QJsonDocument(got).toJson() << "\ninstead.";
    */
#endif
}
