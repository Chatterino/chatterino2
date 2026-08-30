// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/types/All.hpp"
#include "controllers/highlights/types/Common.hpp"
#include "Test.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QString>
#include <QTemporaryDir>

using namespace chatterino;
using namespace chatterino::highlights;
using namespace Qt::StringLiterals;

TEST(HighlightCommon, getID)
{
    MessageHighlight messageHighlight{u"123"};
    ASSERT_EQ(getID(messageHighlight), u"123");

    YourUsernameHighlight yourUsernameHighlight;
    ASSERT_EQ(getID(yourUsernameHighlight), YourUsernameHighlight::ID);
}

TEST(HighlightCommon, getDefaultName)
{
    MessageHighlight messageHighlight{u"123"};
    messageHighlight.setPattern("message pattern");
    messageHighlight.name = "this is not being checked";
    ASSERT_EQ(getDefaultName(messageHighlight), u"message pattern");
    messageHighlight.setPattern("message pattern 2");
    ASSERT_EQ(getDefaultName(messageHighlight), u"message pattern 2");

    YourUsernameHighlight yourUsernameHighlight;
    ASSERT_EQ(getDefaultName(yourUsernameHighlight),
              YourUsernameHighlight::DEFAULT_NAME);
}

TEST(HighlightCommon, getName)
{
    MessageHighlight messageHighlight{u"123"};
    messageHighlight.setPattern("message pattern");
    ASSERT_EQ(getName(messageHighlight), u"message pattern");
    messageHighlight.name = "custom name";
    ASSERT_EQ(getName(messageHighlight), u"custom name");

    YourUsernameHighlight yourUsernameHighlight;
    ASSERT_EQ(getName(yourUsernameHighlight),
              YourUsernameHighlight::DEFAULT_NAME);
    yourUsernameHighlight.name = "custom name";
    ASSERT_EQ(getName(yourUsernameHighlight), u"custom name");

    AnnouncementsHighlight announcements;
    ASSERT_EQ(getName(announcements), AnnouncementsHighlight::DEFAULT_NAME);
}
