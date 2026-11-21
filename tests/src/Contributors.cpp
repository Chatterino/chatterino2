// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "Test.hpp"

#include <QFile>
#include <QPixmap>
#include <QTextStream>

namespace chatterino {

TEST(Contributors, AvatarsReadable)
{
    QFile contributorsFile(":/contributors.txt");
    ASSERT_TRUE(contributorsFile.open(QFile::ReadOnly));

    QTextStream stream(&contributorsFile);

    QString line;

    while (stream.readLineInto(&line))
    {
        if (line.isEmpty() || line.startsWith('#') ||
            line.startsWith(u"@header"))
        {
            continue;
        }

        auto parts = line.split("|");
        ASSERT_EQ(parts.size(), 3)
            << "Contributor line missing 3 parts: " << line;

        auto avatarUrl = parts[2].trimmed();
        if (!avatarUrl.isEmpty())
        {
            QPixmap avatar;
            ASSERT_TRUE(avatar.load(avatarUrl))
                << "Contributor avatar " << avatarUrl << " not loadable";
        }
    }

    QPixmap anonAvatar;
    ASSERT_TRUE(anonAvatar.load(":/avatars/anon.png"));
}

}  // namespace chatterino
