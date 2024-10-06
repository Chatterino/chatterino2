#include "messages/layouts/MessageLayoutContainer.hpp"

#include "common/Literals.hpp"
#include "messages/Emote.hpp"
#include "messages/layouts/MessageLayoutContext.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"
#include "messages/Message.hpp"
#include "messages/MessageElement.hpp"
#include "mocks/BaseApplication.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Theme.hpp"
#include "Test.hpp"

#include <memory>
#include <vector>

using namespace chatterino;
using namespace literals;

namespace {

class MockApplication : mock::BaseApplication
{
public:
    MockApplication()
        : theme(this->paths_)
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

using TestParam = std::tuple<QString, QString, TextDirection>;

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
    auto [inputText, expected, expectedDirection] = GetParam();
    MessageLayoutContainer container;
    MessageLayoutContext ctx{
        .messageColors = {},
        .flags =
            {
                MessageElementFlag::Text,
                MessageElementFlag::Username,
                MessageElementFlag::TwitchEmote,
            },
        .width = 10000,
        .scale = 1.0F,
        .imageScale = 1.0F,
    };
    container.beginLayout(ctx.width, ctx.scale, ctx.imageScale,
                          {MessageFlag::Collapsed});

    auto elements = makeElements(inputText);
    for (const auto &element : elements)
    {
        element->addToContainer(container, ctx);
    }
    container.endLayout();
    ASSERT_EQ(container.line_, 1) << "unexpected linebreak";

    int x = -1;
    for (const auto &el : container.elements_)
    {
        ASSERT_LT(x, el->getRect().x());
        x = el->getRect().x();
    }

    QString got;
    for (const auto &el : container.elements_)
    {
        if (!got.isNull())
        {
            got.append(' ');
        }

        if (dynamic_cast<ImageLayoutElement *>(el.get()))
        {
            el->addCopyTextToString(got);
            if (el->hasTrailingSpace())
            {
                got.chop(1);
            }
        }
        else
        {
            got.append(el->getText());
        }
    }

    ASSERT_EQ(got, expected) << got;
    ASSERT_EQ(container.textDirection_, expectedDirection) << got;
}

INSTANTIATE_TEST_SUITE_P(
    MessageLayoutContainer, MessageLayoutContainerTest,
    testing::Values(
        TestParam{
            u"@aliens foo bar baz @foo qox !emote1 !emote2"_s,
            u"@aliens foo bar baz @foo qox !emote1 !emote2"_s,
            TextDirection::LTR,
        },
        TestParam{
            u"@aliens ! foo bar baz @foo qox !emote1 !emote2"_s,
            u"@aliens ! foo bar baz @foo qox !emote1 !emote2"_s,
            TextDirection::LTR,
        },
        TestParam{
            u"@aliens ."_s,
            u"@aliens ."_s,
            TextDirection::Neutral,
        },
        // RTL
        TestParam{
            u"@aliens و غير دارت إعادة, بل كما وقام قُدُماً. قام تم الجوي بوابة, خلاف أراض هو بلا. عن وحتّى ميناء غير"_s,
            u"@aliens غير ميناء وحتّى عن بلا. هو أراض خلاف بوابة, الجوي تم قام قُدُماً. وقام كما بل إعادة, دارت غير و"_s,
            TextDirection::RTL,
        },
        TestParam{
            u"@aliens و غير دارت إعادة, بل ض هو my LTR 123 بلا. عن 123 456 وحتّى ميناء غير"_s,
            u"@aliens غير ميناء وحتّى 456 123 عن بلا. my LTR 123 هو ض بل إعادة, دارت غير و"_s,
            TextDirection::RTL,
        },
        TestParam{
            u"@aliens ور دارت إ @user baz bar عاد هو my LTR 123 بلا. عن 123 456 وحتّ غير"_s,
            u"@aliens غير وحتّ 456 123 عن بلا. my LTR 123 هو عاد baz bar @user إ دارت ور"_s,
            TextDirection::RTL,
        },
        TestParam{
            u"@aliens ور !emote1 !emote2 !emote3 دارت إ @user baz bar عاد هو my LTR 123 بلا. عن 123 456 وحتّ غير"_s,
            u"@aliens غير وحتّ 456 123 عن بلا. my LTR 123 هو عاد baz bar @user إ دارت !emote3 !emote2 !emote1 ور"_s,
            TextDirection::RTL,
        },
        TestParam{
            u"@aliens ور !emote1 !emote2 LTR text !emote3 !emote4 غير"_s,
            u"@aliens غير LTR text !emote3 !emote4 !emote2 !emote1 ور"_s,
            TextDirection::RTL,
        },

        TestParam{
            u"@aliens !!! ور !emote1 !emote2 LTR text !emote3 !emote4 غير"_s,
            u"@aliens غير LTR text !emote3 !emote4 !emote2 !emote1 ور !!!"_s,
            TextDirection::RTL,
        },
        // LTR
        TestParam{
            u"@aliens LTR و غير دا ميناء غير"_s,
            u"@aliens LTR غير ميناء دا غير و"_s,
            TextDirection::LTR,
        },
        TestParam{
            u"@aliens LTR و غير د ض هو my LTR 123 بلا. عن 123 456 وحتّى مير"_s,
            u"@aliens LTR هو ض د غير و my LTR 123 مير وحتّى 456 123 عن بلا."_s,
            TextDirection::LTR,
        },
        TestParam{
            u"@aliens LTR ور دارت إ @user baz bar عاد هو my LTR 123 بلا. عن 123 456 وحتّ غير"_s,
            u"@aliens LTR @user إ دارت ور baz bar هو عاد my LTR 123 غير وحتّ 456 123 عن بلا."_s,
            TextDirection::LTR,
        },
        TestParam{
            u"@aliens LTR ور !emote1 !emote2 !emote3 دارت إ @user baz bar عاد هو my LTR 123 بلا. عن 123 456 وحتّ غير"_s,
            u"@aliens LTR @user إ دارت !emote3 !emote2 !emote1 ور baz bar هو عاد my LTR 123 غير وحتّ 456 123 عن بلا."_s,
            TextDirection::LTR,
        },
        TestParam{
            u"@aliens LTR غير وحتّ !emote1 !emote2 LTR text !emote3 !emote4 عاد هو"_s,
            u"@aliens LTR !emote2 !emote1 وحتّ غير LTR text !emote3 !emote4 هو عاد"_s,
            TextDirection::LTR,
        }));

}  // namespace chatterino
