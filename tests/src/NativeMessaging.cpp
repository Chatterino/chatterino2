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
