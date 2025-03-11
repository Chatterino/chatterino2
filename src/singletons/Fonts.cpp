#include "singletons/Fonts.hpp"

#include "Application.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"

#include <QDebug>
#include <QtGlobal>

namespace {

using namespace chatterino;

int getUsernameBoldness()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // From qfont.cpp
    // https://github.com/qt/qtbase/blob/589c6d066f84833a7c3dda1638037f4b2e91b7aa/src/gui/text/qfont.cpp#L143-L169
    static constexpr std::array<std::array<int, 2>, 9> legacyToOpenTypeMap{{
        {0, QFont::Thin},
        {12, QFont::ExtraLight},
        {25, QFont::Light},
        {50, QFont::Normal},
        {57, QFont::Medium},
        {63, QFont::DemiBold},
        {75, QFont::Bold},
        {81, QFont::ExtraBold},
        {87, QFont::Black},
    }};

    const int target = getSettings()->boldScale.getValue();

    int result = QFont::Medium;
    int closestDist = INT_MAX;

    // Go through and find the closest mapped value
    for (const auto [weightOld, weightNew] : legacyToOpenTypeMap)
    {
        const int dist = qAbs(weightOld - target);
        if (dist < closestDist)
        {
            result = weightNew;
            closestDist = dist;
        }
        else
        {
            // Break early since following values will be further away
            break;
        }
    }

    return result;
#else
    return getSettings()->boldScale.getValue();
#endif
}

float fontSize(FontStyle style)
{
    auto chatSize = [] {
        return static_cast<float>(getSettings()->chatFontSize);
    };
    switch (style)
    {
        case FontStyle::ChatSmall:
            return 0.6F * chatSize();
        case FontStyle::ChatMediumSmall:
            return 0.8F * chatSize();
        case FontStyle::ChatMedium:
        case FontStyle::ChatMediumBold:
        case FontStyle::ChatMediumItalic:
            return chatSize();
        case FontStyle::ChatLarge:
            return 1.2F * chatSize();
        case FontStyle::ChatVeryLarge:
            return 1.4F * chatSize();

        case FontStyle::Tiny:
            return 8;
        case FontStyle::UiMedium:
        case FontStyle::UiMediumBold:
        case FontStyle::UiTabs:
        case FontStyle::EndType:
            return 9;
    }

    assert(false);
    return 9;
}

int fontWeight(FontStyle style)
{
    switch (style)
    {
        case FontStyle::ChatSmall:
        case FontStyle::ChatMediumSmall:
        case FontStyle::ChatMedium:
        case FontStyle::ChatMediumItalic:
        case FontStyle::ChatLarge:
        case FontStyle::ChatVeryLarge:
            return getSettings()->chatFontWeight.getValue();

        case FontStyle::ChatMediumBold:
            return getUsernameBoldness();

        case FontStyle::Tiny:
        case FontStyle::UiMedium:
        case FontStyle::UiTabs:
        case FontStyle::EndType:
            return QFont::Normal;

        case FontStyle::UiMediumBold:
            return QFont::Bold;
    }

    assert(false);
    return QFont::Normal;
}

bool isItalic(FontStyle style)
{
    switch (style)
    {
        case FontStyle::Tiny:
        case FontStyle::ChatSmall:
        case FontStyle::ChatMediumSmall:
        case FontStyle::ChatMedium:
        case FontStyle::ChatMediumBold:
        case FontStyle::ChatLarge:
        case FontStyle::ChatVeryLarge:
        case FontStyle::UiMedium:
        case FontStyle::UiMediumBold:
        case FontStyle::UiTabs:
        case FontStyle::EndType:
            return false;

        case FontStyle::ChatMediumItalic:
            return true;
    }

    assert(false);
    return false;
}

QString fontFamily(FontStyle style)
{
    switch (style)
    {
        case FontStyle::Tiny:
            return QStringLiteral("Monospace");

        case FontStyle::ChatSmall:
        case FontStyle::ChatMediumSmall:
        case FontStyle::ChatMedium:
        case FontStyle::ChatMediumBold:
        case FontStyle::ChatMediumItalic:
        case FontStyle::ChatLarge:
        case FontStyle::ChatVeryLarge:
            return getSettings()->chatFontFamily.getValue();

        case FontStyle::UiMedium:
        case FontStyle::UiMediumBold:
        case FontStyle::UiTabs:
        case FontStyle::EndType:
            return QStringLiteral(DEFAULT_FONT_FAMILY);
    }

    assert(false);
    return QStringLiteral(DEFAULT_FONT_FAMILY);
}

}  // namespace

namespace chatterino {

Fonts::Fonts(Settings &settings)
{
    this->fontsByType_.resize(size_t(FontStyle::EndType));

    this->fontChangedListener.setCB([this] {
        assertInGuiThread();

        for (auto &map : this->fontsByType_)
        {
            map.clear();
        }
        this->fontChanged.invoke();
    });
    this->fontChangedListener.addSetting(settings.chatFontFamily);
    this->fontChangedListener.addSetting(settings.chatFontSize);
    this->fontChangedListener.addSetting(settings.chatFontWeight);
    this->fontChangedListener.addSetting(settings.boldScale);
}

QFont Fonts::getFont(FontStyle type, float scale)
{
    return this->getOrCreateFontData(type, scale).font;
}

QFontMetrics Fonts::getFontMetrics(FontStyle type, float scale)
{
    return this->getOrCreateFontData(type, scale).metrics;
}

Fonts::FontData &Fonts::getOrCreateFontData(FontStyle type, float scale)
{
    assertInGuiThread();

    assert(type < FontStyle::EndType);

    auto &map = this->fontsByType_[size_t(type)];

    // find element
    auto it = map.find(scale);
    if (it != map.end())
    {
        // return if found

        return it->second;
    }

    // emplace new element
    auto result = map.emplace(scale, Fonts::createFontData(type, scale));
    assert(result.second);

    return result.first->second;
}

Fonts::FontData Fonts::createFontData(FontStyle type, float scale)
{
    return QFont{
        fontFamily(type),
        static_cast<int>(fontSize(type) * scale),
        fontWeight(type),
        isItalic(type),
    };
}

}  // namespace chatterino
