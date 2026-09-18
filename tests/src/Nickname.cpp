// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/nicknames/Nickname.hpp"

#include "controllers/nicknames/NicknamesModel.hpp"
#include "mocks/BaseApplication.hpp"
#include "mocks/UserData.hpp"
#include "Test.hpp"
#include "widgets/settingspages/NicknamesPage.hpp"

#include <QApplication>
#include <QLineEdit>
#include <QPointer>
#include <QTableView>

using namespace chatterino;

namespace {

class NicknameApplication : public mock::BaseApplication
{
public:
    IUserDataController *getUserData() override
    {
        return &this->userData;
    }

    mock::UserDataController userData;
};

}  // namespace

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

TEST(Nickname, RegexCanRemoveText)
{
    Nickname nickname{"lol$", {}, true, false};

    EXPECT_EQ(nickname.match("forsenlol"), "forsen");
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

TEST(Nickname, UsernameUpdatePreservesActiveEditor)
{
    mock::BaseApplication app;
    mock::UserDataController userData;
    userData.setUserNickname("11148817", "pajlada_old", "Pie Ladder");
    NicknamesModel model{&userData, nullptr};
    model.initialize(&app.settings.nicknames);
    QTableView view;
    view.setModel(&model);
    view.show();

    // Start editing the nickname before the last seen username is updated.
    const auto row = model.rowCount({}) - 1;
    const auto nicknameIndex = model.index(row, 3);
    view.edit(nicknameIndex);
    QApplication::processEvents();
    QPointer<QLineEdit> editor = view.findChild<QLineEdit *>();
    ASSERT_FALSE(editor.isNull());
    editor->setText("unfinished nickname");

    userData.updateLastSeenUsername("11148817", "pajlada");
    QApplication::processEvents();

    // The unfinished nickname remains in the editor.
    ASSERT_FALSE(editor.isNull());
    EXPECT_EQ(editor->text(), "unfinished nickname");

    // The new username is displayed in the row.
    EXPECT_EQ(model.data(model.index(row, 1), Qt::DisplayRole), "pajlada");
}

TEST(Nickname, AccountRowsUseUserData)
{
    mock::BaseApplication app;
    mock::UserDataController userData;
    const auto legacyEntries = app.settings.nicknames.raw().size();
    userData.setUserNickname("11148817", "pajlada", "Pie Ladder");
    NicknamesModel model{&userData, nullptr};
    model.initialize(&app.settings.nicknames);
    QTableView view;
    view.setModel(&model);
    view.show();
    QApplication::processEvents();
    const auto row = model.rowCount({}) - 1;

    int dataChanged = 0;
    QObject::connect(&model, &QAbstractItemModel::dataChanged, [&dataChanged] {
        ++dataChanged;
    });

    // Editing an account row updates the stored nickname.
    ASSERT_TRUE(model.setData(model.index(row, 3), "paja", Qt::EditRole));
    const auto updated = userData.getUser("11148817");
    ASSERT_TRUE(updated);
    EXPECT_EQ(updated.value_or(UserData{}).nickname, "paja");

    // Model observers are notified of the edit.
    EXPECT_GT(dataChanged, 0);

    // Removing the account row clears the stored nickname.
    ASSERT_TRUE(model.removeRow(row));
    QApplication::processEvents();
    EXPECT_FALSE(userData.getUser("11148817"));

    // The removed row disappears from the table.
    EXPECT_EQ(model.rowCount({}), static_cast<int>(legacyEntries));
}

TEST(Nickname, CancelRestoresAccountNickname)
{
    NicknameApplication app;
    const auto legacyEntries = app.settings.nicknames.raw().size();
    app.userData.setUserNickname("11148817", "pajlada", "Pie Ladder");
    app.userData.setUserNotes("11148817", "old note");
    NicknamesPage page;
    auto *table = page.findChild<QTableView *>();
    ASSERT_NE(table, nullptr);
    // NOLINTNEXTLINE(clazy-unneeded-cast)
    auto *model = dynamic_cast<NicknamesModel *>(table->model());
    ASSERT_NE(model, nullptr);
    const auto row = model->rowCount({}) - 1;

    ASSERT_TRUE(model->removeRow(row));
    app.settings.nicknames.append(Nickname{"pajlada", "paja", false, false});
    app.userData.setUserNotes("11148817", "new note");

    // Cancelling restores the account nickname.
    page.onSettingsDialogRejected();
    const auto restored = app.userData.getUser("11148817");
    ASSERT_TRUE(restored);
    const auto restoredData = restored.value_or(UserData{});
    EXPECT_EQ(restoredData.nickname, "Pie Ladder");

    // The account row is restored in the table.
    EXPECT_EQ(model->rowCount({}), row + 1);

    // The added legacy entry is removed.
    EXPECT_EQ(app.settings.nicknames.raw().size(), legacyEntries);

    // Unrelated user data is left unchanged.
    EXPECT_EQ(restoredData.notes, "new note");
}

TEST(Nickname, CancelRestoresLegacyNickname)
{
    NicknameApplication app;
    const auto legacyEntries = app.settings.nicknames.raw().size();
    app.settings.nicknames.append(
        Nickname{"pajlada", "Pie Ladder", false, false});
    NicknamesPage page;
    auto *table = page.findChild<QTableView *>();
    ASSERT_NE(table, nullptr);
    // NOLINTNEXTLINE(clazy-unneeded-cast)
    auto *model = dynamic_cast<NicknamesModel *>(table->model());
    ASSERT_NE(model, nullptr);

    model->setAccountNickname("11148817", "pajlada", "paja");
    app.settings.nicknames.removeAt(static_cast<int>(legacyEntries));

    // Cancelling removes the new account nickname.
    page.onSettingsDialogRejected();
    EXPECT_FALSE(app.userData.getUser("11148817"));

    // The removed legacy entry is restored.
    ASSERT_EQ(app.settings.nicknames.raw().size(), legacyEntries + 1);
    EXPECT_EQ(app.settings.nicknames.readOnly()->back().name(), "pajlada");
    EXPECT_EQ(app.settings.nicknames.readOnly()->back().replace(),
              "Pie Ladder");
}

TEST(Nickname, InlineEditUpdatesOriginalEntry)
{
    mock::BaseApplication app;
    mock::UserDataController userData;
    app.settings.nicknames.append(
        Nickname{"pajlada", "Pie Ladder", false, false});
    NicknamesModel model{&userData, nullptr};
    model.initialize(&app.settings.nicknames);
    const auto row = model.rowCount({}) - 1;

    // Editing a legacy row updates the original nickname entry.
    ASSERT_TRUE(model.setData(model.index(row, 1), "pajlada2", Qt::EditRole));

    const auto entry = model.entryAt(row);
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry.value_or(NicknameEntry{}).username, "pajlada2");
    EXPECT_EQ(app.settings.nicknames.readOnly()->back().name(), "pajlada2");
}
