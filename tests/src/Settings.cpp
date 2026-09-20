// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "singletons/Settings.hpp"

#include "common/Env.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/highlights/types/All.hpp"
#include "lib/Snapshot.hpp"
#include "messages/Emote.hpp"
#include "messages/Message.hpp"
#include "mocks/EmptyApplication.hpp"
#include "providers/twitch/TwitchBadges.hpp"
#include "Test.hpp"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

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
    chatterino::env::readBool("CHATTERINO_UPDATE_TEST_SNAPSHOTS", false);

class MigrationApplication : public mock::EmptyApplication
{
public:
    MigrationApplication(const QString &settingsData)
        : mock::EmptyApplication(settingsData)
        , settings(this->modes_, this->args_, this->settingsDir.path(),
                   {
                       .isTest = true,
                       .runMigrations = true,
                       .runCleanup = false,
                   })
        , updates(this->modes_, this->paths_, this->settings)
    {
    }

    Updates &getUpdates() override
    {
        return this->updates;
    }

    Settings settings;
    Updates updates;
};

class CleanupApplication : public mock::EmptyApplication
{
public:
    CleanupApplication(const QString &settingsData)
        : mock::EmptyApplication(settingsData)
        , settings(this->modes_, this->args_, this->settingsDir.path(),
                   {
                       .isTest = true,
                       .runMigrations = true,
                       .runCleanup = true,
                   })
        , updates(this->modes_, this->paths_, this->settings)
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

class TestSettingsMigration : public ::testing::TestWithParam<QString>
{
public:
    static const QString TEST_CATEGORY;

    void SetUp() override
    {
        auto param = TestSettingsMigration::GetParam();
        this->snapshot = testlib::Snapshot::read(TEST_CATEGORY, param);
    }

    void TearDown() override
    {
        this->snapshot.reset();
    }

    std::unique_ptr<testlib::Snapshot> snapshot;
};

const QString TestSettingsMigration::TEST_CATEGORY = u"Settings/migration"_s;

class TestSettingsCleanup : public ::testing::TestWithParam<QString>
{
public:
    static const QString TEST_CATEGORY;

    void SetUp() override
    {
        auto param = TestSettingsCleanup::GetParam();
        this->snapshot = testlib::Snapshot::read(TEST_CATEGORY, param);
    }

    void TearDown() override
    {
        this->snapshot.reset();
    }

    std::unique_ptr<testlib::Snapshot> snapshot;
};

const QString TestSettingsCleanup::TEST_CATEGORY = u"Settings/cleanup"_s;

TEST_P(TestSettingsMigration, Run)
{
    QJsonObject got;

    MigrationApplication app(
        QJsonDocument(this->snapshot->input().toObject()).toJson());

    QFile settingsFile(app.settingsDir.filePath("settings.json"));
    ASSERT_TRUE(settingsFile.open(QFile::ReadOnly))
        << "failed to open" << app.settingsDir.filePath("settings.json");
    auto content = settingsFile.readAll();
    settingsFile.close();

    got = QJsonDocument::fromJson(content).object();

    if (!snapshot->run(got, UPDATE_SNAPSHOTS))
    {
        // The snapshot failed - using ASSERT_EQ here to try to get some better output
        EXPECT_EQ(QJsonDocument(snapshot->output().toObject()).toJson(),
                  QJsonDocument(got).toJson())
            << "Snapshot " << snapshot->name() << " comparison";
        FAIL() << "Snapshot " << snapshot->name() << " failed";
    }
}

INSTANTIATE_TEST_SUITE_P(SettingsMigration, TestSettingsMigration,
                         testing::ValuesIn(testlib::Snapshot::discover(
                             TestSettingsMigration::TEST_CATEGORY)));

TEST(SettingsMigration, Integrity)
{
    ASSERT_FALSE(UPDATE_SNAPSHOTS);  // make sure fixtures are actually tested
}

TEST_P(TestSettingsCleanup, Run)
{
    QJsonObject got;

    CleanupApplication app(
        QJsonDocument(this->snapshot->input().toObject()).toJson());

    QFile settingsFile(app.settingsDir.filePath("settings.json"));
    ASSERT_TRUE(settingsFile.open(QFile::ReadOnly))
        << "failed to open" << app.settingsDir.filePath("settings.json");
    auto content = settingsFile.readAll();
    settingsFile.close();

    got = QJsonDocument::fromJson(content).object();

    if (!snapshot->run(got, UPDATE_SNAPSHOTS))
    {
        // The snapshot failed - using ASSERT_EQ here to try to get some better output
        EXPECT_EQ(QJsonDocument(snapshot->output().toObject()).toJson(),
                  QJsonDocument(got).toJson())
            << "Snapshot " << snapshot->name() << " comparison";
        FAIL() << "Snapshot " << snapshot->name() << " failed";
    }
}

INSTANTIATE_TEST_SUITE_P(SettingsCleanup, TestSettingsCleanup,
                         testing::ValuesIn(testlib::Snapshot::discover(
                             TestSettingsCleanup::TEST_CATEGORY)));

TEST(SettingsCleanup, Integrity)
{
    ASSERT_FALSE(UPDATE_SNAPSHOTS);  // make sure fixtures are actually tested
}

TEST(Settings, DefaultHighlightSerialization)
{
    using namespace chatterino::highlights;

    rapidjson::Document d;
    auto &a = d.GetAllocator();

    {
        // A default-initialized variant will contain the first
        AllHighlights highlight;
        auto v = pajlada::Serialize<AllHighlights>::get(highlight, a);
        ASSERT_EQ(R"({"id":"yourusername"})", rj::stringify(v));
    }

    {
        // Should be the same as a default-initialized variant
        AllHighlights highlight = YourUsernameHighlight();
        auto v = pajlada::Serialize<AllHighlights>::get(highlight, a);
        ASSERT_EQ(R"({"id":"yourusername"})", rj::stringify(v));
    }

    {
        AllHighlights highlight = AutomodCaughtHighlight();
        auto v = pajlada::Serialize<AllHighlights>::get(highlight, a);
        ASSERT_EQ(R"({"id":"automodcaught"})", rj::stringify(v));
    }

    {
        AllHighlights highlight = MessageHighlight(u"test");
        auto v = pajlada::Serialize<AllHighlights>::get(highlight, a);
        ASSERT_EQ(R"({"id":"test","type":"message"})", rj::stringify(v));

        bool error = false;
        AllHighlights out = pajlada::Deserialize<AllHighlights>::get(v, &error);
        ASSERT_FALSE(error);
        ASSERT_EQ(highlight.index(), out.index());
    }
}
