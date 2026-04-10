// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "singletons/NativeMessaging.hpp"

#include "Test.hpp"
#include "util/CombinePath.hpp"

#include <QDir>
#include <QJsonDocument>
#include <QTemporaryDir>

using namespace chatterino;

using namespace chatterino::nm::detail;

class NativeMessagingFixture : public ::testing::Test
{
    QTemporaryDir tempDir;

protected:
    NativeMessagingFixture()
        : dir(combinePath(this->tempDir.path(), QString("native-messaging")))
    {
    }

    QDir dir;
};

TEST_F(NativeMessagingFixture, writeManifestToSubDirCreated)
{
    // writeManifestTo should succeed if the subdir directory is created
    ASSERT_TRUE(this->dir.mkpath("."));

    ASSERT_TRUE(writeManifestTo(this->dir.path(), "native-messaging-hosts",
                                "test.json", QJsonDocument())
                    .has_value());

    QDir nmDir = combinePath(this->dir.path(), "native-messaging-hosts");

    ASSERT_TRUE(nmDir.exists());

    ASSERT_TRUE(QFile(combinePath(nmDir.path(), "test.json")).exists());
}

TEST_F(NativeMessagingFixture, writeManifestToSubDirNotCreated)
{
    // writeManifestTo should fail if the subdir is not created
    ASSERT_EQ(writeManifestTo(this->dir.path(), "native-messaging-hosts",
                              "test.json", QJsonDocument())
                  .error(),
              WriteManifestError::FailedToCreateDirectory);
}

TEST_F(NativeMessagingFixture, writeManifestToWindowsCurrentDir)
{
    // writeManifestTo should succeed if the subdir is created and the path to create is "."
    ASSERT_TRUE(this->dir.mkpath("."));

    ASSERT_TRUE(
        writeManifestTo(this->dir.path(), ".", "test.json", QJsonDocument())
            .has_value());

    ASSERT_TRUE(QFile(combinePath(this->dir.path(), "test.json")).exists());
}

#ifndef Q_OS_WIN
TEST(NativeMessaging, parseCustomPath)
{
    ASSERT_EQ(parseCustomPath("/my/custom/path/to/manifest.json"),
              std::optional{"/my/custom/path/to/manifest.json"});

    ASSERT_EQ(parseCustomPath(""), std::optional<QString>{});

    ASSERT_EQ(parseCustomPath("~/path/to/manifest.json"),
              std::optional{QDir::homePath() % "/path/to/manifest.json"});

    ASSERT_EQ(parseCustomPath("relative/path/to/manifest.json"),
              std::optional<QString>{});

#    ifdef Q_OS_LINUX
    ASSERT_EQ(
        parseCustomPath("$XDG_CONFIG_HOME/path/to/manifest.json"),
        std::optional{QDir::homePath() % "/.config/path/to/manifest.json"});

    ASSERT_EQ(parseCustomPath("$XDG_DATA_HOME/path/to/manifest.json"),
              std::optional{QDir::homePath() %
                            "/.local/share/path/to/manifest.json"});
#    endif
}
#endif
