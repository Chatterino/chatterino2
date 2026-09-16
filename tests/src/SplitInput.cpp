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
#include "messages/MessageElement.hpp"
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
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/ResizingTextEdit.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/Scrollbar.hpp"
#include "widgets/splits/Split.hpp"

#include <QApplication>
#include <QCompleter>
#include <QDebug>
#include <QString>

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

std::shared_ptr<Message> makeReplyableMessage(QString id, QString user,
                                              const QString &text = "message")
{
    auto message = std::make_shared<Message>();
    message->id = std::move(id);
    message->loginName = user;
    message->displayName = std::move(user);
    message->serverReceivedTime = QDateTime::currentDateTime();
    message->elements.emplace_back(
        std::make_unique<TextElement>(text, MessageElementFlag::Text));
    return message;
}

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

}  // namespace

TEST_F(SplitInputCompletionTest, SelectReplyTarget)
{
    auto channel = std::make_shared<TwitchChannel>("test");
    this->split.setChannel(IndirectChannel{channel});

    const auto older = makeReplyableMessage("1", "older");
    auto notReplyable = makeReplyableMessage("2", "system");
    notReplyable->flags.set(MessageFlag::System);
    const auto newer = makeReplyableMessage("3", "newer");
    channel->addMessage(older, MessageContext::Repost);
    channel->addMessage(notReplyable, MessageContext::Repost);
    channel->addMessage(newer, MessageContext::Repost);

    auto &view = this->split.getChannelView();
    auto &input = this->split.getInput();

    // Pressing older without an active reply selects the newest message.
    view.navigateReplyTarget(nullptr, ReplyTargetDirection::Older);
    EXPECT_EQ(input.getInputText(), "@newer ");

    // Skip messages that cannot be replied to.
    view.navigateReplyTarget(newer, ReplyTargetDirection::Older);
    EXPECT_EQ(input.getInputText(), "@older ");

    // Pressing older at the oldest message does nothing.
    view.navigateReplyTarget(older, ReplyTargetDirection::Older);
    EXPECT_EQ(input.getInputText(), "@older ");

    // Pressing newer selects the next replyable message.
    view.navigateReplyTarget(older, ReplyTargetDirection::Newer);
    EXPECT_EQ(input.getInputText(), "@newer ");

    // Moving past the newest replyable message clears the reply.
    view.navigateReplyTarget(newer, ReplyTargetDirection::Newer);
    EXPECT_TRUE(input.getInputText().isEmpty());
}

TEST_F(SplitInputCompletionTest, ReplyTargetScrolling)
{
    auto channel = std::make_shared<TwitchChannel>("test");
    this->split.setChannel(IndirectChannel{channel});
    this->split.resize(500, 300);
    this->split.show();
    auto &view = this->split.getChannelView();
    view.resize(500, 250);
    view.show();

    std::vector<MessagePtr> messages;
    for (int i = 0; i < 30; ++i)
    {
        auto message = makeReplyableMessage(QString::number(i), "user");
        messages.push_back(message);
        channel->addMessage(message, MessageContext::Repost);
    }
    QApplication::processEvents();

    auto &scrollbar = view.getScrollBar();
    ASSERT_GT(scrollbar.getPageSize(), 0);
    scrollbar.scrollToBottom();

    // Navigate far enough back to scroll away from the latest messages.
    view.navigateReplyTarget(nullptr, ReplyTargetDirection::Older);
    for (int i = 29; i > 19; --i)
    {
        view.navigateReplyTarget(messages[i], ReplyTargetDirection::Older);
        QApplication::processEvents();
        QApplication::processEvents();
    }

    ASSERT_LT(scrollbar.getDesiredValue(), scrollbar.getBottom());

    // Moving towards newer messages scrolls only far enough to reveal them.
    bool adjustedScroll = false;
    for (int i = 19; i < 29; ++i)
    {
        const auto previousScroll = scrollbar.getDesiredValue();
        view.navigateReplyTarget(messages[i], ReplyTargetDirection::Newer);
        QApplication::processEvents();
        QApplication::processEvents();

        if (scrollbar.getDesiredValue() > previousScroll)
        {
            adjustedScroll = true;
            EXPECT_LT(scrollbar.getDesiredValue(), i + 1);
        }
    }
    EXPECT_TRUE(adjustedScroll);

    // Cancelling after navigating from the bottom returns to the bottom.
    this->split.getInput().setReply(nullptr);
    QApplication::processEvents();
    EXPECT_GE(scrollbar.getDesiredValue(), scrollbar.getBottom());

    // We do not force the view back to the selected target if the user
    // manually scrolls away
    view.navigateReplyTarget(nullptr, ReplyTargetDirection::Older);
    QApplication::processEvents();
    scrollbar.scrollToTop();
    QApplication::processEvents();
    QApplication::processEvents();
    EXPECT_EQ(scrollbar.getDesiredValue(), scrollbar.getMinimum());

    // After manually scrolling, cancelling the reply does not
    // return the view to the bottom
    this->split.getInput().setReply(nullptr);
    QApplication::processEvents();
    EXPECT_EQ(scrollbar.getDesiredValue(), scrollbar.getMinimum());
}

TEST_F(SplitInputCompletionTest, OversizedReplyTargetAlignsToTop)
{
    auto channel = std::make_shared<TwitchChannel>("test");
    this->split.setChannel(IndirectChannel{channel});
    this->split.resize(500, 300);
    this->split.show();
    auto &view = this->split.getChannelView();
    view.resize(500, 100);
    view.show();

    const auto oversized = makeReplyableMessage(
        "1", "oversized", QString("forsen ").repeated(200));
    const auto newer = makeReplyableMessage("2", "newer");
    channel->addMessage(oversized, MessageContext::Repost);
    channel->addMessage(newer, MessageContext::Repost);
    QApplication::processEvents();

    auto &scrollbar = view.getScrollBar();
    scrollbar.scrollToBottom();

    // An oversized target cannot be fully shown, so align it to the top.
    view.navigateReplyTarget(nullptr, ReplyTargetDirection::Older);
    view.navigateReplyTarget(newer, ReplyTargetDirection::Older);
    QApplication::processEvents();
    QApplication::processEvents();

    EXPECT_EQ(scrollbar.getDesiredValue(), scrollbar.getMinimum());
}

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
