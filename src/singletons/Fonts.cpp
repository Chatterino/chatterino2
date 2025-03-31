#include "singletons/Fonts.hpp"

#include "Application.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"

#include <QDebug>
#include <QtGlobal>

namespace {

using namespace chatterino;

int getBoldness()
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
    auto result = map.emplace(scale, this->createFontData(type, scale));
    assert(result.second);

    return result.first->second;
}

Fonts::FontData Fonts::createFontData(FontStyle type, float scale)
{
    auto *settings = getSettings();

    // check if it's a chat (scale the setting)
    if (type >= FontStyle::ChatStart && type <= FontStyle::ChatEnd)
    {
        static std::unordered_map<FontStyle, ChatFontData> sizeScale{
            {FontStyle::ChatSmall, {0.6f, false, QFont::Normal}},
            {FontStyle::ChatMediumSmall, {0.8f, false, QFont::Normal}},
            {FontStyle::ChatMedium, {1, false, QFont::Normal}},
            {FontStyle::ChatMediumBold,
             {1, false, QFont::Weight(getBoldness())}},
            {FontStyle::ChatMediumItalic, {1, true, QFont::Normal}},
            {FontStyle::ChatLarge, {1.2f, false, QFont::Normal}},
            {FontStyle::ChatVeryLarge, {1.4f, false, QFont::Normal}},
        };
        sizeScale[FontStyle::ChatMediumBold] = {1, false,
                                                QFont::Weight(getBoldness())};
        auto data = sizeScale[type];
        return FontData(
            QFont(settings->chatFontFamily.getValue(),
                  int(settings->chatFontSize.getValue() * data.scale * scale),
                  data.weight, data.italic));
    }

    // normal Ui font (use pt size)
    {
        static std::unordered_map<FontStyle, UiFontData> defaultSize{
            {
                FontStyle::Tiny,
                {
                    8,
                    "Monospace",
                    false,
                    QFont::Normal,
                },
            },
            {
                FontStyle::UiMedium,
                {
                    9,
                    DEFAULT_FONT_FAMILY,
                    false,
                    QFont::Normal,
                },
            },
            {
                FontStyle::UiMediumBold,
                {
                    9,
                    DEFAULT_FONT_FAMILY,
                    false,
                    QFont::Bold,
                },
            },
            {
                FontStyle::UiTabs,
                {
                    9,
                    DEFAULT_FONT_FAMILY,
                    false,
                    QFont::Normal,
                },
            },
        };

        UiFontData &data = defaultSize[type];
        QFont font(data.name, int(data.size * scale), data.weight, data.italic);
        return FontData(font);
    }
}

}  // namespace chatterino
