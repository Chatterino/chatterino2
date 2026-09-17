// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/nicknames/Nickname.hpp"

#include "mocks/BaseApplication.hpp"
#include "mocks/UserData.hpp"
#include "Test.hpp"

using namespace chatterino;

TEST(Nickname, UserNicknameSerialization)
{
    UserData user{
        .nickname = "Pie Ladder",
        .lastSeenUsername = "pajlada",
    };

    rapidjson::Document document;
    const auto json =
        pajlada::Serialize<UserData>::get(user, document.GetAllocator());

    bool error = false;
    const auto restored = pajlada::Deserialize<UserData>::get(json, &error);
    EXPECT_FALSE(error);
    EXPECT_EQ(restored.nickname, "Pie Ladder");
    EXPECT_EQ(restored.lastSeenUsername, "pajlada");
}

TEST(Nickname, LegacyRulesOverrideAccountNickname)
{
    mock::BaseApplication app;
    mock::UserDataController userData;
    app.settings.nicknames.append(Nickname{"^pajlada.*$", "paja", true, false});
    userData.setUserNickname("11148817", "pajlada", "Pie Ladder");
    EXPECT_EQ(userData.getUser("11148817").value_or(UserData{}).nickname,
              "Pie Ladder");

    // A matching regex entry takes precedence over the account nickname.
    EXPECT_EQ(app.settings.matchNickname("pajlada2", "11148817", &userData),
              "paja");

    // Without the regex entry, the account nickname is used.
    app.settings.nicknames.removeAt(
        static_cast<int>(app.settings.nicknames.raw().size()) - 1);
    EXPECT_EQ(app.settings.matchNickname("pajlada2", "11148817", &userData),
              "Pie Ladder");

    app.settings.nicknames.append(Nickname{"^pajlada.*$", "paja", true, false});

    // The regex entry also matches users without an account nickname.
    EXPECT_EQ(app.settings.matchNickname("pajlada", "117691339", &userData),
              "paja");

    // Removing the account nickname does not affect the regex entry.
    userData.setUserNickname("11148817", "pajlada2", "");
    EXPECT_FALSE(userData.getUser("11148817"));
    EXPECT_EQ(app.settings.matchNickname("pajlada2", "11148817", &userData),
              "paja");
}
