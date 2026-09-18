// SPDX-FileCopyrightText: 2024 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/splits/SplitInput.hpp"

#include "common/Literals.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/completion/TabCompletionModel.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "controllers/plugins/PluginController.hpp"
#include "messages/Emote.hpp"
#include "mocks/BaseApplication.hpp"
#include "mocks/Channel.hpp"
#include "mocks/EmoteController.hpp"
#include "mocks/TwitchIrcServer.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/ffz/FfzEmotes.hpp"
#include "providers/seventv/SeventvEmotes.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "Test.hpp"
#include "widgets/helper/ResizingTextEdit.hpp"
#include "widgets/listview/GenericListView.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/splits/InputCompletionItem.hpp"
#include "widgets/splits/InputCompletionPopup.hpp"
#include "widgets/splits/Split.hpp"

#include <QApplication>
#include <QCompleter>
#include <QDebug>
#include <QKeyEvent>
#include <QString>

#include <initializer_list>
#include <tuple>

using namespace chatterino;
using ::testing::Exactly;

namespace {

class MockApplication : public mock::BaseApplication
{
public:
    MockApplication()
        : windowManager(this->args_, this->paths_, this->settings, this->theme,
                        this->fonts)
        , commands(this->paths_)
    {
    }

    HotkeyController *getHotkeys() override
    {
        return &this->hotkeys;
    }

    WindowManager *getWindows() override
    {
        return &this->windowManager;
    }

    AccountController *getAccounts() override
    {
        return &this->accounts;
    }

    CommandController *getCommands() override
    {
        return &this->commands;
    }

    EmoteController *getEmotes() override
    {
        return &this->emotes;
    }

    ITwitchIrcServer *getTwitch() override
    {
        return &this->twitch;
    }

    mock::MockTwitchIrcServer twitch;

    BttvEmotes *getBttvEmotes() override
    {
        return &this->bttv;
    }

    FfzEmotes *getFfzEmotes() override
    {
        return &this->ffz;
    }

    SeventvEmotes *getSeventvEmotes() override
    {
        return &this->seventv;
    }

    BttvEmotes bttv;
    FfzEmotes ffz;
    SeventvEmotes seventv;

#ifdef CHATTERINO_HAVE_PLUGINS
    PluginController *getPlugins() override
    {
        return &this->plugins;
    }

    PluginController plugins{this->paths_};
#endif

    HotkeyController hotkeys;
    WindowManager windowManager;
    AccountController accounts;
    CommandController commands;
    mock::EmoteController emotes;
};

class TestSplitInput : public SplitInput
{
public:
    using SplitInput::insertCompletionText;
    using SplitInput::SplitInput;

    void undoInput()
    {
        this->ui_.textEdit->undo();
    }
};

class SplitInputCompletionTest : public ::testing::Test
{
public:
    MockApplication mockApplication;
    Split split{nullptr};
    TestSplitInput input{&this->split};
};

class SplitInputTest
    : public ::testing::TestWithParam<std::tuple<QString, QString>>
{
public:
    SplitInputTest()
        : split(new Split(nullptr))
        , input(this->split)
    {
    }

    MockApplication mockApplication;
    Split *split;
    SplitInput input;
};

class PopupCycleFixture : public ::testing::Test
{
public:
    PopupCycleFixture()
        : input(this->split.getInput())
    {
    }

    void setChannel(ChannelPtr channel)
    {
        this->split.setChannel(IndirectChannel(std::move(channel)));
        this->split.show();
    }

    void setChatters(std::initializer_list<QString> names)
    {
        auto channel = std::make_shared<TwitchChannel>("forsen");
        for (const auto &name : names)
        {
            channel->addRecentChatter(name);
        }
        this->setChannel(channel);
    }

    void setEmotes(std::initializer_list<QString> names, bool zeroWidth = false)
    {
        auto emotes = std::make_shared<EmoteMap>();
        for (const auto &name : names)
        {
            auto emote = std::make_shared<Emote>();
            emote->name = EmoteName{name};
            emote->zeroWidth = zeroWidth;
            emotes->emplace(emote->name, std::move(emote));
        }
        auto channel = std::make_shared<TwitchChannel>("forsen");
        channel->setBttvEmotes(std::move(emotes));
        this->setChannel(channel);
    }

    ResizingTextEdit *edit()
    {
        auto *textEdit = this->input.findChild<QTextEdit *>();
        // NOLINTNEXTLINE(clazy-unneeded-cast)
        return dynamic_cast<ResizingTextEdit *>(textEdit);
    }

    InputCompletionPopup *popup()
    {
        const auto children = this->input.findChildren<QWidget *>();
        for (auto *child : children)
        {
            if (auto *result = dynamic_cast<InputCompletionPopup *>(child))
            {
                return result;
            }
        }
        return nullptr;
    }

    GenericListView *list()
    {
        auto *result = this->popup();
        return result ? result->findChild<GenericListView *>() : nullptr;
    }

    void pressTab()
    {
        QKeyEvent tab(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "\t");
        QApplication::sendEvent(this->edit(), &tab);
    }

    void pressBacktab()
    {
        QKeyEvent backtab(QEvent::KeyPress, Qt::Key_Backtab, Qt::ShiftModifier);
        QApplication::sendEvent(this->edit(), &backtab);
    }

    MockApplication app;
    Split split{nullptr};
    SplitInput &input;
};

}  // namespace

TEST_F(SplitInputCompletionTest, EmoteCompletionPreservesUndoHistory)
{
    this->input.insertText("don't ping him :paja");
    ASSERT_EQ("don't ping him :paja", this->input.getInputText());

    this->input.insertCompletionText("pajaGIGA");
    ASSERT_EQ("don't ping him pajaGIGA ", this->input.getInputText());

    this->input.undoInput();
    EXPECT_EQ("don't ping him :paja", this->input.getInputText());
    this->input.undoInput();
    EXPECT_TRUE(this->input.getInputText().isEmpty());
}

TEST_F(SplitInputCompletionTest, UsernameCompletionPreservesUndoHistory)
{
    this->input.insertText("boring game @fors");
    ASSERT_EQ("boring game @fors", this->input.getInputText());

    this->input.insertCompletionText("forsen");
    ASSERT_EQ("boring game @forsen ", this->input.getInputText());

    this->input.undoInput();
    EXPECT_EQ("boring game @fors", this->input.getInputText());
    this->input.undoInput();
    EXPECT_TRUE(this->input.getInputText().isEmpty());
}

TEST_F(PopupCycleFixture, UsernameSelectionCyclesWithTab)
{
    this->setChatters({"zzpopupfirst", "zzpopupsecond"});
    ASSERT_NE(this->edit(), nullptr);

    this->input.insertText("@zzpopup");
    auto *list = this->list();
    ASSERT_NE(list, nullptr);
    ASSERT_TRUE(this->popup()->isVisible());
    ASSERT_EQ(list->model()->rowCount(), 2);
    list->setCurrentIndex(list->model()->index(1, 0));
    const auto *selected = dynamic_cast<const InputCompletionItem *>(
        GenericListItem::fromVariant(list->currentIndex().data()));
    ASSERT_NE(selected, nullptr);
    const auto selectedText = "@" + selected->insertionText() + ", ";

    // The first tab accepts the selected username
    this->pressTab();
    const auto first = this->input.getInputText();
    EXPECT_EQ(first, selectedText);

    // Further tabs cycle through the popup usernames
    this->pressTab();
    const auto second = this->input.getInputText();
    EXPECT_TRUE(second == "@zzpopupfirst, " || second == "@zzpopupsecond, ");
    EXPECT_NE(second, first);
    this->pressTab();
    EXPECT_EQ(this->input.getInputText(), first);
    this->pressBacktab();
    EXPECT_EQ(this->input.getInputText(), second);
}

TEST_F(PopupCycleFixture, EnterAcceptanceCyclesWithTab)
{
    this->setChatters({"zzpopupfirst", "zzpopupsecond"});
    this->input.insertText("@zzpopup");
    auto *list = this->list();
    ASSERT_NE(list, nullptr);
    ASSERT_EQ(list->model()->rowCount(), 2);
    list->setCurrentIndex(list->model()->index(1, 0));

    // Enter accepts the popup item and tab continues through its list
    QKeyEvent enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, "\r");
    QApplication::sendEvent(this->edit(), &enter);
    const auto first = this->input.getInputText();
    EXPECT_TRUE(first == "@zzpopupfirst, " || first == "@zzpopupsecond, ");
    this->pressTab();
    EXPECT_NE(this->input.getInputText(), first);
}

TEST_F(PopupCycleFixture, OneCharacterSelectionCyclesAfterClick)
{
    this->setEmotes({"⚽", "⚽more"}, true);
    this->input.insertText(":~⚽");
    auto *list = this->list();
    ASSERT_NE(list, nullptr);
    ASSERT_EQ(list->model()->rowCount(), 2);
    const auto *first = dynamic_cast<const InputCompletionItem *>(
        GenericListItem::fromVariant(list->model()->index(0, 0).data()));
    ASSERT_NE(first, nullptr);
    const auto selection = list->model()->index(
        first->insertionText() == QString::fromUtf8("⚽") ? 0 : 1, 0);
    const auto *selected = dynamic_cast<const InputCompletionItem *>(
        GenericListItem::fromVariant(selection.data()));
    ASSERT_NE(selected, nullptr);
    ASSERT_EQ(selected->insertionText(), QString::fromUtf8("⚽"));

    // Clicking a one-character result still allows tab to choose the next
    list->setCurrentIndex(selection);
    Q_EMIT list->clicked(selection);
    EXPECT_EQ(this->input.getInputText(), QString::fromUtf8("⚽ "));
    this->pressTab();
    EXPECT_EQ(this->input.getInputText(), QString::fromUtf8("⚽more "));
}

TEST_F(PopupCycleFixture, CompletionAfterNewlineKeepsPreviousLine)
{
    this->setChatters({"zzpopupfirst", "zzpopupsecond"});
    this->input.insertText("previous\n@zzpopup");
    auto *list = this->list();
    ASSERT_NE(list, nullptr);
    ASSERT_EQ(list->model()->rowCount(), 2);
    list->setCurrentIndex(list->model()->index(0, 0));

    // Cycling a completion on a new line keeps the text before the newline
    this->pressTab();
    const auto first = this->input.getInputText();
    ASSERT_TRUE(first.startsWith("previous\n@"));
    this->pressTab();
    const auto second = this->input.getInputText();
    EXPECT_TRUE(second.startsWith("previous\n@"));
    EXPECT_NE(second, first);
}

TEST_F(PopupCycleFixture, DuplicateEmoteNamesCycleOnce)
{
    this->setEmotes({"zzdupone", "zzduptwo"});
    auto duplicate = std::make_shared<Emote>();
    duplicate->name = EmoteName{"zzdupone"};
    auto globalEmotes = std::make_shared<EmoteMap>();
    globalEmotes->emplace(duplicate->name, duplicate);
    this->app.bttv.setEmotes(std::move(globalEmotes));

    this->input.insertText(":zzdup");
    auto *list = this->list();
    ASSERT_NE(list, nullptr);
    // The same emote name appears in both the channel and global results
    ASSERT_EQ(list->model()->rowCount(), 3);

    int lastDuplicateRow = -1;
    int duplicateCount = 0;
    for (int row = 0; row < list->model()->rowCount(); ++row)
    {
        const auto *item = dynamic_cast<const InputCompletionItem *>(
            GenericListItem::fromVariant(list->model()->index(row, 0).data()));
        ASSERT_NE(item, nullptr);
        if (item->insertionText() == "zzdupone")
        {
            lastDuplicateRow = row;
            ++duplicateCount;
        }
    }
    ASSERT_EQ(duplicateCount, 2);
    ASSERT_GE(lastDuplicateRow, 0);
    list->setCurrentIndex(list->model()->index(lastDuplicateRow, 0));

    // Selecting a duplicate skips the other copy on the next tab
    this->pressTab();
    EXPECT_EQ(this->input.getInputText(), "zzdupone ");
    this->pressTab();
    EXPECT_EQ(this->input.getInputText(), "zzduptwo ");
}

TEST_F(SplitInputCompletionTest, TabCompletionPreservesUndoHistory)
{
    ResizingTextEdit edit;
    QCompleter completer;
    edit.setCompleter(&completer);
    edit.insertPlainText("UR DONE BA");

    Q_EMIT completer.highlighted(QString("BAND "));
    ASSERT_EQ("UR DONE BAND ", edit.toPlainText());

    edit.undo();
    EXPECT_EQ("UR DONE BA", edit.toPlainText());
    edit.undo();
    EXPECT_TRUE(edit.toPlainText().isEmpty());
}

TEST_P(SplitInputTest, Reply)
{
    std::tuple<QString, QString> params = this->GetParam();
    auto [inputText, expected] = params;
    ASSERT_EQ("", this->input.getInputText());
    this->input.setInputText(inputText);
    ASSERT_EQ(inputText, this->input.getInputText());

    auto *message = new Message();
    message->displayName = "forsen";
    auto reply = MessagePtr(message);
    this->input.setReply(reply);
    QString actual = this->input.getInputText();
    ASSERT_EQ(expected, actual) << "Input text after setReply should be '"
                                << expected << "', but got '" << actual << "'";
}

TEST(SplitInput, ReplyCommandCompletion)
{
    MockApplication app;
    app.commands.items.append(Command{"/slashreplycompletion", "test"});
    app.commands.items.append(Command{".dotreplycompletion", "test"});
    Split split(nullptr);
    SplitInput input(&split);
    auto *textEdit = input.findChild<QTextEdit *>();
    // NOLINTNEXTLINE(clazy-unneeded-cast)
    auto *edit = dynamic_cast<ResizingTextEdit *>(textEdit);
    ASSERT_NE(edit, nullptr);
    mock::MockChannel channel("forsen");
    edit->setCompleter(new QCompleter(channel.completionModel, edit));

    // Slash and dot commands can be completed in replies
    for (const auto &trigger :
         {QString("/slashreplycomp"), QString(".dotreplycomp")})
    {
        auto message = std::make_shared<Message>();
        message->displayName = "forsen";
        input.setInputText("");
        input.setReply(message);
        input.insertText(trigger);

        // Commands are completed at the first word after the reply prefix `@username `
        QKeyEvent tab(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
        QApplication::sendEvent(edit, &tab);
        EXPECT_EQ(input.getInputText(), "@forsen " + trigger + "letion ");

        // Commands are not completed later than the first word after the prefix
        input.setInputText("@forsen test " + trigger);
        edit->moveCursor(QTextCursor::End);
        QApplication::sendEvent(edit, &tab);
        EXPECT_EQ(input.getInputText(), "@forsen test " + trigger);

        // Manually restoring an edited prefix does not restore reply completion
        input.setInputText("@forsenX " + trigger);
        input.setInputText("@forsen " + trigger);
        edit->moveCursor(QTextCursor::End);
        QApplication::sendEvent(edit, &tab);
        EXPECT_EQ(input.getInputText(), "@forsen " + trigger);

        // Normal command completion works after cancelling a reply
        input.setReply(message);
        input.setReply(nullptr);
        input.setInputText(trigger);
        edit->moveCursor(QTextCursor::End);
        QApplication::sendEvent(edit, &tab);
        EXPECT_EQ(input.getInputText(), trigger + "letion ");
    }
}

TEST(SplitInput, EmptyReplyUsernameCompletion)
{
    MockApplication app;
    app.settings.mentionUsersWithComma = true;
    app.settings.alwaysIncludeBroadcasterInUserCompletions = true;
    app.commands.items.append(Command{"/slashreplycompletion", "test"});
    Split split(nullptr);
    SplitInput input(&split);
    auto *textEdit = input.findChild<QTextEdit *>();
    // NOLINTNEXTLINE(clazy-unneeded-cast)
    auto *edit = dynamic_cast<ResizingTextEdit *>(textEdit);
    ASSERT_NE(edit, nullptr);
    TwitchChannel channel("forsen");
    edit->setCompleter(new QCompleter(channel.completionModel, edit));

    auto message = std::make_shared<Message>();
    message->displayName = "forsen";
    input.setReply(message);

    // Completing the username in an empty reply does not add a comma
    QKeyEvent tab(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
    QApplication::sendEvent(edit, &tab);
    EXPECT_EQ(input.getInputText(), "@forsen ");

    // Reply completion still works after completing the prefix username
    input.insertText("/slashreplycomp");
    QApplication::sendEvent(edit, &tab);
    EXPECT_EQ(input.getInputText(), "@forsen /slashreplycompletion ");
}

TEST(SplitInput, ReplyBodyUsernameCompletion)
{
    MockApplication app;
    Split split(nullptr);
    SplitInput input(&split);
    auto *textEdit = input.findChild<QTextEdit *>();
    // NOLINTNEXTLINE(clazy-unneeded-cast)
    auto *edit = dynamic_cast<ResizingTextEdit *>(textEdit);
    ASSERT_NE(edit, nullptr);
    TwitchChannel channel("forsen");
    channel.addRecentChatter("pajlada");
    edit->setCompleter(new QCompleter(channel.completionModel, edit));

    auto message = std::make_shared<Message>();
    message->displayName = "forsen";
    input.setReply(message);

    for (bool mentionComma : {true, false})
    {
        app.settings.mentionUsersWithComma = mentionComma;

        // The first username in the reply body respects the comma setting
        input.setInputText("@forsen @paj");
        edit->moveCursor(QTextCursor::End);
        EXPECT_TRUE(input.isEditFirstWord());
        QKeyEvent tab(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
        QApplication::sendEvent(edit, &tab);
        EXPECT_EQ(input.getInputText(),
                  mentionComma ? "@forsen @pajlada, " : "@forsen @pajlada ");

        // Usernames later in the reply body do not get a comma
        input.setInputText("@forsen test @paj");
        edit->moveCursor(QTextCursor::End);
        EXPECT_FALSE(input.isEditFirstWord());
        QApplication::sendEvent(edit, &tab);
        EXPECT_EQ(input.getInputText(), "@forsen test @pajlada ");
    }
}

TEST(SplitInput, ReplyPrefixFormatting)
{
    MockApplication app;
    Split split(nullptr);
    SplitInput input(&split);
    auto *edit = input.findChild<QTextEdit *>();
    ASSERT_NE(edit, nullptr);

    auto message = std::make_shared<Message>();
    message->displayName = "forsen";
    input.setReply(message);
    input.insertText("pretty much everywhere, it's gonna be hot");

    auto prefixFormatting = [&](const QString &prefix) {
        QList<QTextEdit::ExtraSelection> result;
        const auto selections = edit->extraSelections();
        for (const auto &selection : selections)
        {
            if (selection.cursor.selectionStart() == 0 &&
                selection.cursor.selectionEnd() == prefix.size())
            {
                result.append(selection);
            }
        }
        return result;
    };

    // Reply prefix is set when replying
    ASSERT_EQ(prefixFormatting("@forsen ").size(), 1);
    EXPECT_EQ(prefixFormatting("@forsen ").constFirst().cursor.selectedText(),
              "@forsen ");

    // Reply prefix uses the theme's placeholder colour
    EXPECT_EQ(
        prefixFormatting("@forsen ").constFirst().format.foreground().color(),
        app.theme.messages.textColors.chatPlaceholder);

    // The cursor stays in position after the message
    EXPECT_EQ(edit->textCursor().position(), input.getInputText().size());

    // Changing the reply target updates the prefix
    auto other = std::make_shared<Message>();
    other->displayName = "pajlada";
    input.setReply(other);
    ASSERT_EQ(prefixFormatting("@pajlada ").size(), 1);
    EXPECT_EQ(prefixFormatting("@pajlada ").constFirst().cursor.selectedText(),
              "@pajlada ");

    // Editing the prefix invalidates it
    input.setInputText("@pajladaX pajaBing");
    EXPECT_TRUE(prefixFormatting("@pajlada ").isEmpty());

    // Restoring the prefix manually does not restore it on our end
    input.setInputText("@pajlada pajaBing");
    EXPECT_TRUE(prefixFormatting("@pajlada ").isEmpty());

    // Cancelling the reply invalidates the prefix
    input.setReply(message);
    input.setReply(nullptr);
    EXPECT_TRUE(prefixFormatting("@forsen ").isEmpty());

    // Clearing the input invalidates the reply prefix
    input.setReply(message);
    input.setInputText("");
    EXPECT_TRUE(prefixFormatting("@forsen ").isEmpty());
}

INSTANTIATE_TEST_SUITE_P(
    SplitInput, SplitInputTest,
    testing::Values(
        // Ensure message is retained
        std::make_tuple<QString, QString>(
            // Pre-existing text in the input
            "Test message",
            // Expected text after replying to forsen
            "@forsen Test message "),

        // Ensure mention is stripped, no message
        std::make_tuple<QString, QString>(
            // Pre-existing text in the input
            "@forsen",
            // Expected text after replying to forsen
            "@forsen "),

        // Ensure mention with space is stripped, no message
        std::make_tuple<QString, QString>(
            // Pre-existing text in the input
            "@forsen ",
            // Expected text after replying to forsen
            "@forsen "),

        // Ensure mention is stripped, retain message
        std::make_tuple<QString, QString>(
            // Pre-existing text in the input
            "@forsen Test message",
            // Expected text after replying to forsen
            "@forsen Test message "),

        // Ensure mention with comma is stripped, no message
        std::make_tuple<QString, QString>(
            // Pre-existing text in the input
            "@forsen,",
            // Expected text after replying to forsen
            "@forsen "),

        // Ensure mention with comma is stripped, retain message
        std::make_tuple<QString, QString>(
            // Pre-existing text in the input
            "@forsen Test message",
            // Expected text after replying to forsen
            "@forsen Test message "),

        // Ensure mention with comma and space is stripped, no message
        std::make_tuple<QString, QString>(
            // Pre-existing text in the input
            "@forsen, ",
            // Expected text after replying to forsen
            "@forsen "),

        // Ensure it works with no message
        std::make_tuple<QString, QString>(
            // Pre-existing text in the input
            "",
            // Expected text after replying to forsen
            "@forsen ")));
