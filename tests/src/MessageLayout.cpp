// SPDX-FileCopyrightText: 2024 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "messages/layouts/MessageLayout.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "messages/layouts/MessageLayoutContext.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"
#include "messages/MessageBuilder.hpp"
#include "messages/MessageElement.hpp"
#include "mocks/BaseApplication.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "Test.hpp"

#include <QDebug>
#include <QString>

#include <memory>

using namespace chatterino;

namespace {

class MockApplication : public mock::BaseApplication
{
public:
    MockApplication()
        : windowManager(this->args_, this->paths_, this->settings, this->theme,
                        this->fonts)
    {
    }

    WindowManager *getWindows() override
    {
        return &this->windowManager;
    }

    AccountController *getAccounts() override
    {
        return &this->accounts;
    }

    AccountController accounts;
    WindowManager windowManager;
};

constexpr int WIDTH = 300;

class MessageLayoutTest
{
public:
    // "aaaaaaaa bbbbbbbb cccccccc"
    MessageLayoutTest(const QString &text)
    {
        MessageBuilder builder;
        builder.append(
            std::make_unique<TextElement>(text, MessageElementFlag::Text));
        this->layout = std::make_unique<MessageLayout>(builder.release());
        MessageColors colors;
        this->layout->layout(
            {
                .messageColors = colors,
                .flags = MessageElementFlag::Text,
                .width = WIDTH,
                .scale = 1,
                .imageScale = 1,
            },
            false);
    }

    MockApplication mockApplication;
    std::unique_ptr<MessageLayout> layout;
};

}  // namespace

TEST(TextElement, BasicCase)
{
    auto test = MessageLayoutTest("abc");

    // Simulate we are clicking on the first word
    auto point = QPoint(WIDTH / 20, test.layout->getHeight() / 2);

    const auto *hoveredElement = test.layout->getElementAt(point);
    ASSERT_NE(hoveredElement, nullptr);

    const auto [wordStart, wordEnd] =
        test.layout->getWordBounds(hoveredElement, point);

    EXPECT_EQ(wordStart, 0);
    EXPECT_EQ(wordEnd, 3);
}

TEST(EmoteElement, ToggleIgnoreInExistingMessage)
{
    MockApplication app;
    QPixmap pixmap(20, 20);
    pixmap.fill(Qt::red);
    auto emote = std::make_shared<Emote>();
    emote->name = EmoteName{"forsenE"};
    emote->tooltip = Tooltip{"forsenE"};
    emote->images.setImage1(Image::fromResourcePixmap(pixmap));
    MessageBuilder builder;
    auto *source =
        builder.emplace<EmoteElement>(emote, MessageElementFlag::Emote);
    source->setLink({Link::InsertText, "forsenE"});
    source->setTrailingSpace(false);
    builder.emplace<TextElement>("!", MessageElementFlag::Text);
    MessageLayout layout(builder.release());
    MessageColors colors;
    const auto relayout = [&] {
        layout.flags.set(MessageLayoutFlag::RequiresLayout);
        layout.layout(
            {
                .messageColors = colors,
                .flags = MessageElementFlags{MessageElementFlag::EmoteImage,
                                             MessageElementFlag::Text},
                .width = WIDTH,
                .scale = 1,
                .imageScale = 1,
            },
            false);
        return layout.getElementAt(QPoint(WIDTH / 20, layout.getHeight() / 2));
    };

    ASSERT_NE(dynamic_cast<const ImageLayoutElement *>(relayout()), nullptr);
    app.settings.setEmoteNameIgnored("forsenE", true);
    const auto *text = relayout();
    // Ignoring an emote already in chat renders it as text.
    ASSERT_NE(dynamic_cast<const TextLayoutElement *>(text), nullptr);
    EXPECT_FALSE(text->isImage());
    // The text keeps the emote's tooltip and insertion link.
    EXPECT_EQ(&text->getCreator(), source);
    EXPECT_EQ(text->getCreator().getTooltip(), "forsenE");
    EXPECT_EQ(text->getLink().type, Link::InsertText);
    EXPECT_EQ(text->getText(), "forsenE");
    // The replacement keeps the emote adjacent to the following text.
    EXPECT_FALSE(text->hasTrailingSpace());
    QString copied;
    layout.addSelectionText(copied);
    EXPECT_EQ(copied, "forsenE!");
    // The text retains the emote's flags for context actions.
    EXPECT_TRUE(text->getFlags().has(MessageElementFlag::EmoteImage));

    app.settings.setEmoteNameIgnored("forsenE", false);
    // Removing the ignore renders the image again.
    EXPECT_NE(dynamic_cast<const ImageLayoutElement *>(relayout()), nullptr);

    // Ignored emotes remain images in the picker.
    source->addFlags(MessageElementFlag::AlwaysShow);
    app.settings.setEmoteNameIgnored("forsenE", true);
    EXPECT_NE(dynamic_cast<const ImageLayoutElement *>(relayout()), nullptr);
}

TEST(EmoteElement, UnavailableImageUsesTextLayout)
{
    MockApplication app;
    auto emote = std::make_shared<Emote>();
    emote->name = EmoteName{"forsenE"};
    MessageBuilder builder;
    builder.emplace<EmoteElement>(emote, MessageElementFlag::Emote);
    MessageLayout layout(builder.release());
    MessageColors colors;
    layout.layout(
        {
            .messageColors = colors,
            .flags = MessageElementFlag::EmoteImage,
            .width = WIDTH,
            .scale = 1,
            .imageScale = 1,
        },
        false);

    const auto *element =
        layout.getElementAt(QPoint(WIDTH / 20, layout.getHeight() / 2));
    // An unavailable image uses the ordinary text fallback.
    ASSERT_NE(dynamic_cast<const TextLayoutElement *>(element), nullptr);
    EXPECT_FALSE(element->isImage());
    EXPECT_FALSE(element->getFlags().has(MessageElementFlag::EmoteImage));
}
