#include "util/XDGDesktopFile.hpp"

#include <gtest/gtest.h>
#include <QDebug>

using namespace chatterino;

TEST(XDGDesktopFile, String)
{
    auto desktopFile = XDGDesktopFile(":/001-mimeapps.list");
    auto entries = desktopFile.getEntries("Default Applications");

    ASSERT_EQ(entries["thisshould"], "");

    ASSERT_EQ(entries["lol"], "");
    ASSERT_EQ(entries["x-scheme-handler/http"], QString("firefox.desktop"));

    ASSERT_EQ(desktopFile.getEntries("test").size(), 2);
}
