#include "messages/layouts/MessageLayoutContainer.hpp"

#include "common/Literals.hpp"
#include "messages/Emote.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"
#include "messages/Message.hpp"
#include "messages/MessageElement.hpp"
#include "mocks/EmptyApplication.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "Test.hpp"

#include <memory>
#include <vector>

using namespace chatterino;
using namespace literals;

namespace {

class MockApplication : mock::EmptyApplication
{
public:
    MockApplication()
        : settings(this->settingsDir.filePath("settings.json"))
        , theme(this->paths_)
        , fonts(this->settings)
    {
    }
    Theme *getThemes() override
    {
        return &this->theme;
    }

    Fonts *getFonts() override
    {
        return &this->fonts;
    }

    Settings settings;
    Theme theme;
    Fonts fonts;
};

std::vector<std::shared_ptr<MessageElement>> makeElements(const QString &text)
{
    std::vector<std::shared_ptr<MessageElement>> elements;
    bool seenUsername = false;
    for (const auto &word : text.split(' '))
    {
        if (word.startsWith('@'))
        {
            if (seenUsername)
            {
                elements.emplace_back(std::make_shared<MentionElement>(
                    word, word, MessageColor{}, MessageColor{}));
            }
            else
            {
                elements.emplace_back(std::make_shared<TextElement>(
                    word, MessageElementFlag::Username, MessageColor{},
                    FontStyle::ChatMediumBold));
                seenUsername = true;
            }
            continue;
        }

        if (word.startsWith('!'))
        {
            auto emote = std::make_shared<Emote>(Emote{
                .name = EmoteName{word},
                .images = ImageSet{Image::fromResourcePixmap(
                    getResources().buttons.addSplit)},
                .tooltip = {},
                .homePage = {},
                .id = {},
                .author = {},
                .baseName = {},
            });
            elements.emplace_back(std::make_shared<EmoteElement>(
                emote, MessageElementFlag::TwitchEmote));
            continue;
        }

        elements.emplace_back(std::make_shared<TextElement>(
            word, MessageElementFlag::Text, MessageColor{},
            FontStyle::ChatMedium));
    }

    return elements;
}

using TestParam = std::tuple<QString, QString>;

}  // namespace

namespace chatterino {

class MessageLayoutContainerTest : public ::testing::TestWithParam<TestParam>
{
public:
    MessageLayoutContainerTest() = default;

    MockApplication mockApplication;
};

TEST_P(MessageLayoutContainerTest, RtlReordering)
{
    auto [inputText, expected] = GetParam();
    MessageLayoutContainer container;
    container.beginLayout(10000, 1.0F, 1.0F, {MessageFlag::Collapsed});

    auto elements = makeElements(inputText);
    for (const auto &element : elements)
    {
        element->addToContainer(container, {
                                               MessageElementFlag::Text,
                                               MessageElementFlag::Username,
                                               MessageElementFlag::TwitchEmote,
                                           });
    }
    container.breakLine();
    ASSERT_EQ(container.line_, 1) << "unexpected linebreak";

    // message layout elements ordered by x position
    std::vector<MessageLayoutElement *> ordered;
    ordered.reserve(container.elements_.size());
    for (const auto &el : container.elements_)
    {
        ordered.push_back(el.get());
    }

    std::ranges::sort(ordered, [](auto *a, auto *b) {
        return a->getRect().x() < b->getRect().x();
    });

    QString got;
    for (const auto &el : ordered)
    {
        if (!got.isNull())
        {
            got.append(' ');
        }

        if (dynamic_cast<ImageLayoutElement *>(el))
        {
            el->addCopyTextToString(got);
            ASSERT_TRUE(got.endsWith(' '));
            got.chop(1);
        }
        else
        {
            got.append(el->getText());
        }
    }

    ASSERT_EQ(got, expected) << got;
}

INSTANTIATE_TEST_SUITE_P(
    MessageLayoutContainer, MessageLayoutContainerTest,
    testing::Values(
        TestParam{
            u"@aliens foo bar baz @foo qox !emote1 !emote2"_s,
            u"@aliens foo bar baz @foo qox !emote1 !emote2"_s,
        },
        // RTL
        TestParam{
            u"@aliens و غير دارت إعادة, بل كما وقام قُدُماً. قام تم الجوي بوابة, خلاف أراض هو بلا. عن وحتّى ميناء غير"_s,
            u"@aliens غير ميناء وحتّى عن بلا. هو أراض خلاف بوابة, الجوي تم قام قُدُماً. وقام كما بل إعادة, دارت غير و"_s,
        },
        TestParam{
            u"@aliens و غير دارت إعادة, بل ض هو my LTR 123 بلا. عن 123 456 وحتّى ميناء غير"_s,
            u"@aliens غير ميناء وحتّى 456 123 عن بلا. my LTR 123 هو ض بل إعادة, دارت غير و"_s,
        },
        TestParam{
            u"@aliens ور دارت إ @user baz bar عاد هو my LTR 123 بلا. عن 123 456 وحتّ غير"_s,
            u"@aliens غير وحتّ 456 123 عن بلا. my LTR 123 هو عاد baz bar @user إ دارت ور"_s,
        },
        TestParam{
            u"@aliens ور !emote1 !emote2 !emote3 دارت إ @user baz bar عاد هو my LTR 123 بلا. عن 123 456 وحتّ غير"_s,
            u"@aliens غير وحتّ 456 123 عن بلا. my LTR 123 هو عاد baz bar @user إ دارت !emote3 !emote2 !emote1 ور"_s,
        },
        TestParam{
            u"@aliens ور !emote1 !emote2 LTR text !emote3 !emote4 غير"_s,
            u"@aliens غير LTR text !emote3 !emote4 !emote2 !emote1 ور"_s,
        },
        // LTR
        TestParam{
            u"@aliens LTR و غير دا ميناء غير"_s,
            u"@aliens LTR غير ميناء دا غير و"_s,
        },
        TestParam{
            u"@aliens LTR و غير د ض هو my LTR 123 بلا. عن 123 456 وحتّى مير"_s,
            u"@aliens LTR هو ض د غير و my LTR 123 مير وحتّى 456 123 عن بلا."_s,
        },
        TestParam{
            u"@aliens LTR ور دارت إ @user baz bar عاد هو my LTR 123 بلا. عن 123 456 وحتّ غير"_s,
            u"@aliens LTR @user إ دارت ور baz bar هو عاد my LTR 123 غير وحتّ 456 123 عن بلا."_s,
        },
        TestParam{
            u"@aliens LTR ور !emote1 !emote2 !emote3 دارت إ @user baz bar عاد هو my LTR 123 بلا. عن 123 456 وحتّ غير"_s,
            u"@aliens LTR @user إ دارت !emote3 !emote2 !emote1 ور baz bar هو عاد my LTR 123 غير وحتّ 456 123 عن بلا."_s,
        },
        TestParam{
            u"@aliens LTR غير وحتّ !emote1 !emote2 LTR text !emote3 !emote4 عاد هو"_s,
            u"@aliens LTR !emote2 !emote1 وحتّ غير LTR text !emote3 !emote4 هو عاد"_s,
        }));

}  // namespace chatterino
